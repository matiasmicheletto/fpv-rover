#include "config.h"
#include "util.h"

// Prototypes

#ifdef CAM_ENABLED
  void startCameraServer(int& port);
#endif

#ifdef WSS_ENABLED
  WebSocketsServer webSocketSvr(WSS_PORT);
  uint8_t client_num; // Number of clients 
  bool wss_connected = false;
#endif

#ifdef DISPLAY_ENABLED
  LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS); 
#endif

// Global variables
int leftSetpoint = PWM_MAX, rightSetpoint = PWM_MAX; // Left and right (0..255)
int leftOutput = PWM_MAX, rightOutput = PWM_MAX; // Left and right outputs (0..255)
int watchdog = 0; // Output updates counter

#ifdef WSS_ENABLED
  void webSocketEvent(uint8_t client, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
      case WStype_CONNECTED: {
        IPAddress ip = webSocketSvr.remoteIP(client);
        Serial.printf("[%u] WSS connection from IP: %d.%d.%d.%d\n", client, ip[0], ip[1], ip[2], ip[3]); 
        wss_connected = true;
        break;
      }
      case WStype_DISCONNECTED:{
        Serial.printf("[%u] WSS disconected!\n", client);
        wss_connected = false;
        break;
      }
      case WStype_TEXT:{
        client_num = client;
        // Payload must have 6 digit from 000 to 255
        // Convert both values to integer and update setpoints
        const int pd = PAYLOAD_DIGITS/2;
        leftSetpoint = parseValue((char*) payload, pd);
        rightSetpoint = parseValue((char*) payload + pd, pd);
        #ifdef RS232_DEBUG
          Serial.printf("New setpoints: (%d,%d) \n", leftSetpoint, rightSetpoint);
        #endif
        watchdog = 0;
        break;
      }
    }
  }
#endif


void update_outputs() {
  watchdog++;
  if(watchdog >= MAX_WATCHDOG){ // If no input, stop rover
    leftSetpoint = PWM_MAX;
    rightSetpoint = PWM_MAX;
    watchdog = 0;
  }

  // Exponential smoothing acceleration
  const int newLeftOutput = (int) (INERT * (float) leftSetpoint + (1-INERT) * (float) leftOutput);
  const int newRightOutput = (int) (INERT * (float) rightSetpoint + (1-INERT) * (float) rightOutput);

#ifdef PWM_OUTPUTS_ENABLED
  // Update outputs
  if(!areEqual(leftOutput, newLeftOutput)){
    if(leftOutput >= PWM_MAX){
      analogWrite(FL_PIN, leftOutput-PWM_MAX);
      analogWrite(BL_PIN, 0);
    }else{
      analogWrite(FL_PIN, 0);
      analogWrite(BL_PIN, PWM_MAX-leftOutput);
    }
    leftOutput = newLeftOutput;
  }

  if(!areEqual(rightOutput, newRightOutput)){
    if(rightOutput >= PWM_MAX){
      analogWrite(FR_PIN, rightOutput-PWM_MAX);
      analogWrite(BR_PIN, 0);
    }else{
      analogWrite(FR_PIN, 0);
      analogWrite(BR_PIN, PWM_MAX-rightOutput);
    }
    rightOutput = newRightOutput;
  }
#endif

#ifdef WSS_ENABLED
    // Send comands as feedback
    if(wss_connected){
      // Verify number of PAYLOAD_DIGITS (here is 6)
      char payload[7];
      sprintf(payload, "%03d%03d", leftOutput, rightOutput); 
      webSocketSvr.sendTXT(client_num, payload); // Send values to client
      #ifdef RS232_DEBUG
        Serial.printf("Sent values: (%d,%d) \n",leftOutput, rightOutput);
      #endif
    }
#endif
}

void readyBlink(){
  analogWrite(FLASH_LED_NUM, 255);
  delay(100);
  analogWrite(FLASH_LED_NUM, 0);
  delay(100);
  analogWrite(FLASH_LED_NUM, 255);
  delay(100);
  analogWrite(FLASH_LED_NUM, 0);
}

void setup() {
  Serial.begin(BAUDRATE);
  Serial.println("Setting up system...");

  pinMode(LED_GPIO_NUM, OUTPUT);
  pinMode(FLASH_LED_NUM, OUTPUT);
  
#ifdef PWM_OUTPUTS_ENABLED
  pinMode(FL_PIN, OUTPUT);
  pinMode(FR_PIN, OUTPUT);
  pinMode(BL_PIN, OUTPUT);
  pinMode(BR_PIN, OUTPUT);
#endif

#ifdef DISPLAY_ENABLED
  lcd.init();
  lcd.backlight(); 
#endif

#ifdef CAM_ENABLED
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
    Serial.printf("PSRAM found\n");
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else { // Limit the frame size when PSRAM is not available
    Serial.printf("PSRAM not found\n");
    config.frame_size = FRAMESIZE_SVGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }else{
    Serial.printf("Camera initialized\n");
  }

  sensor_t *s = esp_camera_sensor_get();
  s->set_quality(s, 63);
  s->set_agc_gain(s, 16);
  s->set_whitebal(s, 1);
  s->set_gain_ctrl(s, 0);
  s->set_aec_value(s, 6);
#endif

#ifdef WIFI_ENABLED
  Serial.printf("Initializing WiFi connection");
  bool connected = false;
  IPAddress ip;
  int tries = 0;
  while(!connected){
    WiFi.begin(ssid, password);
    WiFi.setSleep(false);
    while (WiFi.status() != WL_CONNECTED && tries < WIFI_CONNECT_ATTEMPTS) {
      digitalWrite(LED_GPIO_NUM, LOW);
      delay(250);
      Serial.printf(".");
      digitalWrite(LED_GPIO_NUM, HIGH);
      delay(250);
      tries++;
    }
    ip = WiFi.localIP();
    if(ip[0] != 0)
      connected = true;
     else{
      tries++;
      Serial.printf("\nIP Error, retrying...");
     }
  }
  Serial.printf("Done\n");
#endif

#ifdef WSS_ENABLED
  webSocketSvr.begin();
  webSocketSvr.onEvent(webSocketEvent);
  Serial.printf("Web Socket Server listening on http://%d.%d.%d.%d:%d\n", ip[0], ip[1], ip[2], ip[3], WSS_PORT);
#endif

#if defined(CAM_ENABLED) && defined(WIFI_ENABLED)
  int port = CAM_PORT;
  startCameraServer(port); // Get streaming port
  s->set_xclk(s, LEDC_TIMER_0, 40); // After boot, set 40MHz for clock freq
  Serial.printf("Camera stream URL: http://");
  Serial.printf("%d.%d.%d.%d:%d/stream \n", ip[0], ip[1], ip[2], ip[3], port); 
#endif

  readyBlink();
}

void loop() {
#ifdef WSS_ENABLED
    webSocketSvr.loop();
#endif
  update_outputs();
  delay(100);
}
