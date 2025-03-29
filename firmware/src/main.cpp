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

WebSocketsServer webSocketSvr(WSS_PORT);  
uint8_t client_num; // Number of clients 
bool wss_connected = false;
void webSocketEvent(uint8_t client, WStype_t type, uint8_t * payload, size_t length);

// LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS); 

// Ouput pwm values goes from 0 to PWM_MAX (255 for 8-bit PWM)
// Setpoints and speed values go from -PWM_MAX to PWM_MAX (negative values for backward)
// Client setpoints and feedback values are in the range 0 to 2*PWM_MAX
int leftSetpoint = 0; // Left setpoint and rotation direction <- left motor speed sent by client. Values between -PWM_MAX and PWM_MAX
int rightSetpoint = 0; // Right setpoint and rotation direction <- right motor speed sent by client. Values between -PWM_MAX and PWM_MAX
int leftSpeed = 0; // Left motor speed and rotation direction. Values between -PWM_MAX and PWM_MAX
int rightSpeed = 0; // Right motor speed and rotation direction. Values between -PWM_MAX and PWM_MAX

 // Output updating counter. Resets outputs if no input, so client must keep controlling rover
int watchdog = 0;

// Flash control
bool flashBlinking = false;
bool flashOn = false;
int flashCounter = 0;

/* Timer interrupt variables for real time control of tasks
 * The operating system consists on 3 tasks: 
    - Web socket server loop: 20ms (2 slots), 
    - Motor control outputs update: 50ms (5 slots)
    - Flash led blinking period: 200ms (20 slots)
*/
volatile uint32_t timerticks[3] = {0,0,0}; 
hw_timer_t * timer = NULL;

int parseValue(char * ptr, int size) { // Convert char array to integer
  int value = 0;
  for (int i = 0; i < size; i++) {
    if (ptr[i] < '0' || ptr[i] > '9') // Error handling
      return 0; 
    value = value * 10 + (ptr[i] - '0'); 
  }
  return value;
}

/* Less efficient implementation of the parseValue function. 
 * This one works, but I keep it here in case the previous one doesn't
int parseValue(char * ptr, int size) { // Convert char array to integer
  char value[size+1];
  memcpy(value, ptr, size);
  value[size] = '\0';
  return atoi(value);
}
*/


void IRAM_ATTR onTimer() {
  timerticks[0]++; // 20ms
  timerticks[1]++; // 50ms
  timerticks[2]++; // 200ms
}


// Websocket server events callback
void webSocketEvent(uint8_t client, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_CONNECTED: { // Client connected -> get IP and print it on serial port
      IPAddress ip = webSocketSvr.remoteIP(client);
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
      if(length != PAYLOAD_DIGITS){
        DEBUG_PRINTF("Invalid payload length: %d\n", length);
        break;
      }
      // Convert both values to integer and update setpoints
      const int pd = PAYLOAD_DIGITS/2;
      leftSetpoint = parseValue((char*) payload, pd) - PWM_MAX;
      rightSetpoint = parseValue((char*) payload + pd, pd) - PWM_MAX;
      DEBUG_PRINTF("New setpoints: (%d,%d) \n", leftSetpoint, rightSetpoint);
      watchdog = 0; // Reset watchdog when updating setpoints
      break;
    }
  }
}


void flashBlink(){ // Start blinking flash LED
  flashBlinking = true;
}


void updateFlash() { // Update flash LED state
  if(flashBlinking){
    if(flashCounter >= 3){ // After 3 loops, stop blinking
      flashBlinking = false;
      flashCounter = 0;
      analogWrite(FLASH_LED_NUM, 0);
    }else{ // Alternate between on and off
      analogWrite(FLASH_LED_NUM, flashOn ? PWM_MAX : 0);
      flashOn = !flashOn;
      flashCounter++;
    }
  }
}


void updateSpeed() {
  
  watchdog++;

  if(watchdog >= MAX_WATCHDOG){ // If no input after a set of loops, stop motors
    leftSetpoint = 0;
    rightSetpoint = 0;
    watchdog = 0;
    //flashBlink(); // Blink flash LED to indicate motor stopping
  }

  // Exponential smoothing acceleration
  leftSpeed = (int) (INERT * (float) leftSetpoint + (1-INERT) * (float) leftSpeed);
  rightSpeed = (int) (INERT * (float) rightSetpoint + (1-INERT) * (float) rightSpeed);

  // Update outputs
  if(leftSpeed >= 0){ // Left motor forward
    analogWrite(FL_PIN, leftSpeed);
    analogWrite(BL_PIN, 0);
  }else{ // Left motor backward
    analogWrite(FL_PIN, 0);
    analogWrite(BL_PIN, PWM_MAX + leftSpeed);
  }

  if(rightSpeed >= 0){ // Right motor forward
    analogWrite(FR_PIN, rightSpeed);
    analogWrite(BR_PIN, 0);
  }else{ // Right motor backward
    analogWrite(FR_PIN, 0);
    analogWrite(BR_PIN, PWM_MAX + rightSpeed);
  }
    
  // Send comands as feedback
  if(wss_connected){
    // Verify number of PAYLOAD_DIGITS (here is 6)
    char payload[7];
    const int leftFeedback = leftSpeed + PWM_MAX;
    const int rightFeedback = rightSpeed + PWM_MAX;
    sprintf(payload, "%03d%03d", leftFeedback, rightFeedback); 
    webSocketSvr.sendTXT(client_num, payload); // Send values to client
    DEBUG_PRINTF("Sent values: (%d,%d) \n", leftFeedback, rightFeedback);
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
    while (WiFi.status() != WL_CONNECTED && tries < WIFI_CONNECT_ATTEMPTS) {
      digitalWrite(LED_GPIO_NUM, LOW);
      delay(250);
      DEBUG_PRINTF(".");
      digitalWrite(LED_GPIO_NUM, HIGH);
      delay(250);
      tries++;
    }
    ip = WiFi.localIP();
    if(ip[0] != 0)
      connected = true;
     else{
      tries++;
      DEBUG_PRINTF("\nIP Error, retrying...");
     }
  }
  DEBUG_PRINTF("Done\n");

  webSocketSvr.begin();
  webSocketSvr.onEvent(webSocketEvent);
  DEBUG_PRINTF("Web Socket Server listening on http://%d.%d.%d.%d:%d\n", ip[0], ip[1], ip[2], ip[3], WSS_PORT);

  int port = CAM_PORT;
  startCameraServer(port); // Get streaming port
  s->set_xclk(s, LEDC_TIMER_0, 40); // After boot, set 40MHz for clock freq
  DEBUG_PRINTF("Camera stream URL: http://");
  DEBUG_PRINTF("%d.%d.%d.%d:%d/stream \n", ip[0], ip[1], ip[2], ip[3], port); 

  // Configure Timer 2 (Timer Group 1, Timer 2)
  timer = timerBegin(2, 80, true);  // Timer 2, prescaler 80 -> 1 tick = 1μs
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 10000, true); // Interrupt every 10ms (10000µs)
  timerAlarmEnable(timer); // Enable timer

  flashBlink();
}

void loop() {

  // Time slot is 10ms

  if(timerticks[0] >= 2){ // 20ms
    timerticks[0] = 0;
    webSocketSvr.loop();
  }

  if(timerticks[1] >= 10){ // 50ms
    timerticks[1] = 0;
    updateSpeed();  
  }

  if(timerticks[2] >= 20){ // 200ms
    timerticks[2] = 0;
    updateFlash();
  }
}
