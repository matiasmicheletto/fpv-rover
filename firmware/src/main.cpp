#include <WiFi.h>
#include "WebSocketsServer.h"
#include "esp_camera.h"
//#include "LiquidCrystal_I2C.h"
#include "credentials.h" // WiFi credentials
#include "config.h"
#include "esp_timer.h"

#ifdef RS232_DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(...)
#endif

void startCameraServer(int& port); // Prototype

WebSocketsServer web_socker_server(WSS_PORT);  
uint8_t client_num; // Number of clients 
bool wss_connected = false;
void web_socket_event(uint8_t client, WStype_t type, uint8_t * payload, size_t length);

// LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS); 

// Ouput pwm values goes from 0 to PWM_MAX (255 for 8-bit PWM)
// Setpoints and speed values go from -PWM_MAX to PWM_MAX (negative values for backward)
// Client setpoints and feedback values are in the range 0 to 2*PWM_MAX
int left_setpoint = 0; // Left setpoint and rotation direction <- left motor speed sent by client. Values between -PWM_MAX and PWM_MAX
int right_setpoint = 0; // Right setpoint and rotation direction <- right motor speed sent by client. Values between -PWM_MAX and PWM_MAX
int left_speed = 0; // Left motor speed and rotation direction. Values between -PWM_MAX and PWM_MAX
int right_speed = 0; // Right motor speed and rotation direction. Values between -PWM_MAX and PWM_MAX

 // Output updating counter. Resets outputs if no input, so client must keep controlling rover
int watchdog = 0;

// Flash control
bool flash_blinking = false;
bool flash_on = false;
int flash_counter = 0;
int max_flash_count = 3; // Number of times to blink (negative values for infinite)
volatile uint32_t flash_blink_period = 25; // default period is 250ms

/* Timer interrupt variables
 * The real-time operating system consists on 4 tasks: 
    - 0: Web socket server loop: 10ms (1 slot), 
    - 1: Motor control outputs update: 20ms (2 slots)
    - 2: Feedback update: 100ms (10 slots)
    - 3: Flash led blinking period: 250ms (25 slots) by default, but configurable
*/
volatile uint32_t timerticks[4] = {0, 0, 0, 0}; 
hw_timer_t * timer = NULL;


int parse_value(char * ptr, int size) { // Convert char array to integer
  int value = 0;
  for (int i = 0; i < size; i++) {
    if (ptr[i] < '0' || ptr[i] > '9') // Error handling
      return 0; 
    value = value * 10 + (ptr[i] - '0'); 
  }
  return value;
}


int clamp_motor_speed(int speed) { // Clamp low speeds to prevent motor stalling
  return speed < CLAMP_THRES ? 0 : speed;
}


int get_speed(int setpoint, int speed) { // Exponential smoothing acceleration
  return (int) (INERT * (float) setpoint + (1-INERT) * (float) speed);
}


void IRAM_ATTR on_timer_tick() {
  timerticks[0]++; // Web socket server loop
  timerticks[1]++; // Motor control outputs update
  timerticks[2]++; // Feedback update
  timerticks[3]++; // Flash led blinking period  
}


// Websocket server events callback
void web_socket_event(uint8_t client, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_CONNECTED: { // Client connected -> get IP and print it on serial port
      IPAddress ip = web_socker_server.remoteIP(client);
      DEBUG_PRINTF("[%u] WSS connection from IP: %d.%d.%d.%d\n", client, ip[0], ip[1], ip[2], ip[3]);
      wss_connected = true;
      break;
    }
    case WStype_DISCONNECTED:{ // Client disconnected, closed browser, not WiFi, etc.
      DEBUG_PRINTF("[%u] WSS disconected!\n", client);
      wss_connected = false;
      break;
    }
    case WStype_TEXT:{ // Client sent a message
      client_num = client;
      // Payload must have 6 digit from 000 to 2*PWM_MAX (510 for 8-bit PWM)
      if(length != 6){
        DEBUG_PRINTF("Invalid payload length: %d\n", length);
        break;
      }
      // Convert both values to integer and update setpoints
      const int pd = 3; // Number of digits in payload for each value
      left_setpoint = parse_value((char*) payload, pd) - PWM_MAX;
      right_setpoint = parse_value((char*) payload + pd, pd) - PWM_MAX;
      DEBUG_PRINTF("New setpoints: (%d,%d) \n", left_setpoint, right_setpoint);
      watchdog = 0; // Reset watchdog when updating setpoints
      break;
    }
  }
}


void send_feedback() { // Send current motor speeds to client
  if(wss_connected){
    const int left_feedback = left_speed + PWM_MAX;
    const int right_feedback = right_speed + PWM_MAX;
    char payload[7];
    sprintf(payload, "%03d%03d", left_feedback, right_feedback); // Format payload as 6 digits
    web_socker_server.sendTXT(client_num, payload); // Send values to client
    DEBUG_PRINTF("Sent values: (%d,%d) \n", left_feedback, right_feedback);
  }
}


void flash_blink(){ // Start blinking flash LED
  flash_blinking = true;
}


void update_flash() { // Update flash LED state
  if(flash_blinking){
    if(max_flash_count > 0 && flash_counter >= max_flash_count){ // Stop blinking condition
      flash_blinking = false;
      flash_counter = 0;
      analogWrite(FLASH_LED_NUM, 0);
    }else{ // Alternate between on and off
      analogWrite(FLASH_LED_NUM, flash_on ? PWM_MAX : 0);
      flash_on = !flash_on;
      flash_counter++;
    }
  }
}


void critical_error() { // Idle mode
  left_setpoint = 0;
  right_setpoint = 0;
  max_flash_count = -1; // Infinite blinking
  flash_blink_period = 10; // Blink every 100ms
  flash_blink(); // Blink flash LED to indicate motor stopping
  DEBUG_PRINTF("Critical error, waiting for reset\n");
  //while(true){};
}


void update_motors_speed() {

  watchdog++;

  // If no input after a set of loops, stop motors
  // Watchdog is reset when new setpoints are received
  if(watchdog >= MAX_WATCHDOG){ 
    left_setpoint = 0;
    right_setpoint = 0;
    if(left_speed != 0 || right_speed != 0){ // While halting, blink flash LED
      DEBUG_PRINTF("Watchdog timeout, halting rover\n. Current speed: (%d,%d)\n", left_speed, right_speed);
      flash_blink();
    }
    watchdog = MAX_WATCHDOG; // Prevents watchdog overflow
  }

  // Compute motor speeds (PWM value)
  left_speed = get_speed(left_setpoint, left_speed);
  right_speed = get_speed(right_setpoint, right_speed);

  // Update outputs
  if(left_speed >= 0){ // Left motor forward
    analogWrite(FL_PIN, clamp_motor_speed(left_speed));
    analogWrite(BL_PIN, 0);
  }else{ // Left motor backward
    analogWrite(FL_PIN, 0);
    analogWrite(BL_PIN, clamp_motor_speed(PWM_MAX + left_speed));
  }

  if(right_speed >= 0){ // Right motor forward
    analogWrite(FR_PIN, clamp_motor_speed(right_speed));
    analogWrite(BR_PIN, 0);
  }else{ // Right motor backward
    analogWrite(FR_PIN, 0);
    analogWrite(BR_PIN, clamp_motor_speed(PWM_MAX + right_speed));
  }
}


void setup() {
  #ifdef RS232_DEBUG
    Serial.begin(BAUDRATE);
    DEBUG_PRINTF("Setting up system...\n");
  #endif

  pinMode(LED_GPIO_NUM, OUTPUT);
  pinMode(FLASH_LED_NUM, OUTPUT);
  
  // Set ouput modes
  pinMode(FL_PIN, OUTPUT);
  pinMode(FR_PIN, OUTPUT);
  pinMode(BL_PIN, OUTPUT);
  pinMode(BR_PIN, OUTPUT);
  // Disable outputs (set motors off)
  analogWrite(FL_PIN, 0);
  analogWrite(FR_PIN, 0);
  analogWrite(BL_PIN, 0);
  analogWrite(BR_PIN, 0);

  /*
  lcd.init();
  lcd.backlight(); 
  */

  // OV260 config
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  //config.xclk_freq_hz = 10000000;
  config.xclk_freq_hz = 20000000;
  //config.frame_size = FRAMESIZE_QVGA;
  config.frame_size = FRAMESIZE_VGA;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  //config.jpeg_quality = 12;
  config.jpeg_quality = 8;
  config.fb_count = 1;
  config.grab_mode = CAMERA_GRAB_LATEST;
  if(psramFound()){
    DEBUG_PRINTF("PSRAM found\n");
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else { // Limit the frame size when PSRAM is not available
    DEBUG_PRINTF("PSRAM not found\n");
    config.frame_size = FRAMESIZE_SVGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    DEBUG_PRINTF("Camera init failed with error 0x%x", err);
    critical_error();
    return;
  }else{
    DEBUG_PRINTF("Camera initialized\n");
  }

  sensor_t *s = esp_camera_sensor_get();
  s->set_quality(s, 63);
  s->set_agc_gain(s, 16);
  s->set_whitebal(s, 1);
  s->set_gain_ctrl(s, 0);
  s->set_aec_value(s, 6);

  DEBUG_PRINTF("Initializing WiFi connection");
  bool connected = false;
  IPAddress ip;
  int tries = 0;
  while(!connected){
    WiFi.begin(ssid, password);
    WiFi.setSleep(false);
    while (WiFi.status() != WL_CONNECTED) {
      digitalWrite(LED_GPIO_NUM, LOW);
      delay(250);
      DEBUG_PRINTF(".");
      digitalWrite(LED_GPIO_NUM, HIGH);
      delay(250);
      tries++;
    }
    ip = WiFi.localIP();
    if(ip[0] != 0){
      connected = true;
    }else{
      tries++;
      DEBUG_PRINTF("\nIP Error, retrying...");
      if(tries > WIFI_CONNECT_ATTEMPTS){
        DEBUG_PRINTF("Failed to connect to WiFi after %d attempts\n", tries);
        critical_error();
        return;
      }
    }
  }
  DEBUG_PRINTF("Done\n");

  web_socker_server.begin();
  web_socker_server.onEvent(web_socket_event);
  DEBUG_PRINTF("Web Socket Server listening on http://%d.%d.%d.%d:%d\n", ip[0], ip[1], ip[2], ip[3], WSS_PORT);

  int port = CAM_PORT;
  startCameraServer(port); // Get streaming port
  s->set_xclk(s, LEDC_TIMER_0, 40); // After boot, set 40MHz for clock freq
  DEBUG_PRINTF("Camera stream URL: http://");
  DEBUG_PRINTF("%d.%d.%d.%d:%d/stream \n", ip[0], ip[1], ip[2], ip[3], port); 

  // Configure Timer 2 (Timer Group 1, Timer 2)
  timer = timerBegin(2, 80, true);  // Timer 2, prescaler 80 -> 1 tick = 1μs
  timerAttachInterrupt(timer, &on_timer_tick, true);
  timerAlarmWrite(timer, 10000, true); // Interrupt every 10ms (10000µs)
  timerAlarmEnable(timer); // Enable timer

  flash_blink();
}

void loop() {

  // Time slot is 10ms

  if(timerticks[0] >= 1){ // 10ms
    web_socker_server.loop();
    timerticks[0] = 0;
  }

  if(timerticks[1] >= 2){ // 20ms
    update_motors_speed();  
    timerticks[1] = 0;
  }

  if(timerticks[2] >= 10){ // 100ms
    send_feedback();
    timerticks[2] = 0;
  }

  if(timerticks[3] >= flash_blink_period){
    update_flash();
    timerticks[3] = 0;
  }
}
