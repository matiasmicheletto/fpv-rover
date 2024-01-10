#define WIFI_ENABLED 1
#define CAM_ENABLED 1
#define WSS_ENABLED 1
#define PWM_OUTPUTS_ENABLED 1
//#define DISPLAY_ENABLED 1

#ifdef WIFI_ENABLED
  #include <WiFi.h>
  #include "credentials.h"
  /* content of credentials.h
    const char* ssid = "YOUR_NETWORK_SSID";
    const char* password = "YOUR_NETWORK_PASSWORD";
  */
#endif
#ifdef CAM_ENABLED
  #include "esp_camera.h"
#endif
#ifdef WSS_ENABLED
  #include "WebSocketsServer.h"
#endif
#ifdef DISPLAY_ENABLED
  #include "LiquidCrystal_I2C.h"
#endif

// General config
#define BAUDRATE 115200
#define INERT 6 // Acceleration inertia factor
#define MAX_WATCHDOG 10 // Max. number of loops before all stop

// OV260 pins
#ifdef CAM_ENABLED
  #define CAMERA_MODEL_AI_THINKER
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
#endif
#define LED_GPIO_NUM      33
#define FLASH_LED_NUM     4

// H bridge control pins
#ifdef PWM_OUTPUTS_ENABLED
  #define FL_PIN 13 // Left forward --> In_3
  #define BL_PIN 14 // Left backward --> In_4
  #define FR_PIN 15 // Right forward --> In_1
  #define BR_PIN 2 // Right backward --> In_2
#endif

// LCD display configuration
#ifdef DISPLAY_ENABLED
  #define LCD_ADDR 0x27 // Or 0x3F if not working
  #define LCD_COLS 20
  #define LCD_ROWS 4
#endif

// Prototypes
#ifdef CAM_ENABLED
  void startCameraServer(int& port);
#endif

// Params
int spL = 1023, spR = 1023; // Left and right (0..2046)
int pwrL = 1023, pwrR = 1023; // Left and right outputs (0..2046)
int watchdog = 0; // Output updates counter

#ifdef WSS_ENABLED
  #define WSS_PORT 82
  WebSocketsServer webSocketSvr(WSS_PORT);
  uint8_t client_num; // Number of clients 
  bool wss_connected = false;
#endif

#ifdef DISPLAY_ENABLED
  LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS); 
#endif

int parseValue(char * ptr, int size) {
  char val[size+1];
  memcpy(val, ptr, size);
  val[size] = '\0';
  return atoi(val);
}

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
        // Payload must have 8 digit from 0 a 2046
        char val1[5];
        // Convert both values to integer and update setpoints
        spL = parseValue((char*) payload, 4);
        spR = parseValue((char*) payload+4, 4);
        Serial.printf("New setpoints: (%d,%d) \n",spL, spR);
        watchdog = 0;
        break;
      }
    }
  }
#endif


void update_outputs() {
  watchdog++;
  if(watchdog >= MAX_WATCHDOG){ // If no input, stop rover
    spL = 1023;
    spR = 1023;
    watchdog = 0;
  }

  // Smooth acceleration
  pwrL = (spL + INERT*pwrL) / (INERT+1);
  pwrR = (spR + INERT*pwrR) / (INERT+1);

#ifdef PWM_OUTPUTS_ENABLED
  // Update outputs
  if(pwrL >= 1023){
    analogWrite(FL_PIN,pwrL-1023);
    analogWrite(BL_PIN,0);
  }else{
    analogWrite(FL_PIN,0);
    analogWrite(BL_PIN,1023-pwrL);
  }

  if(pwrR >= 1023){
    analogWrite(FR_PIN,pwrR-1023);
    analogWrite(BR_PIN,0);
  }else{
    analogWrite(FR_PIN,0);
    analogWrite(BR_PIN,1023-pwrR);
  }
#endif

#ifdef WSS_ENABLED
    // Send comands as feedback
    if(wss_connected){
      char payload[9];
      sprintf(payload, "%04d%04d", pwrL, pwrR);
      webSocketSvr.sendTXT(client_num, payload); // Send values to client
      Serial.printf("Sent values: (%d,%d) \n",pwrL, pwrR);
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
  config.xclk_freq_hz = 10000000;
  config.frame_size = FRAMESIZE_QVGA;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
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
    while (WiFi.status() != WL_CONNECTED && tries < 20) {
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
  int port; startCameraServer(port); // Get streaming port (should be 80)
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
  delay(200);
}
