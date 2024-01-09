#define WSS_ENABLED true
#define DISPLAY_ENABLED true
#define TICKER_ENABLED true

#include <WiFi.h>
#include "esp_camera.h"

#ifdef WSS_ENABLED
  #include "wss/WebSocketsServer.h"
#endif
#ifdef TICKER_ENABLED
  #include "Ticker.h"
#endif
#ifdef DISPLAY_ENABLED
  #include <Wire.h>
  #include "LiquidCrystal_I2C.h"
#endif

#include "credentials.h"
/* content of credentials.h
  const char* ssid = "YOUR_NETWORK_SSID";
  const char* password = "YOUR_NETWORK_PASSWORD";
*/

// General config
#define BAUDRATE 115200
#define T_PERIOD 200 // Output updating period (ms)
#define INERT 6 // Acceleration inertia factor
#define MAX_WATCHDOG 10 // Max. number of loops before all stop

// OV260 pins
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
#define LED_GPIO_NUM       4

// H bridge control pins
#define FL_PIN 12 // Left forward --> In_3
#define BL_PIN 13 // Left backward --> In_4
#define FR_PIN 16 // Right forward --> In_1
#define BR_PIN 2 // Right backward --> In_2

// LCD display configuration
#define SDA 15
#define SCL 14
#define LCD_ADDR 0x27 // Or 0x3F if not working
#define LCD_COLS 20
#define LCD_ROWS 4

// Prototypes
void startCameraServer(int& port);
void setupLedFlash(int pin);

int spL = 1023, spR = 1023; // Left and right (0..2046)
int pwrL = 1023, pwrR = 1023; // Left and right outputs (0..2046)
int watchdog = 0; // Loop counter

#ifdef WSS_ENABLED
  WebSocketsServer *webSocketSvr;
  uint8_t client_num; // Number of clients 
  bool wss_connected = false;
#endif

#ifdef DISPLAY_ENABLED
  LiquidCrystal_I2C *lcd; 
#endif

#ifdef WSS_ENABLED
  void webSocketEvent(uint8_t client, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
      case WStype_CONNECTED: {
        IPAddress ip = webSocketSvr->remoteIP(client);
        Serial.printf("[%u] WSS connection URL: %d.%d.%d.%d - %s\n", client, ip[0], ip[1], ip[2], ip[3], payload); 
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
        char val1[4] = {(char)payload[0],(char)payload[1],(char)payload[2],(char)payload[3]};
        char val2[4] = {(char)payload[4],(char)payload[5],(char)payload[6],(char)payload[7]};
        // Convert values to integer and set endpoints
        spL = atoi(val1);
        spR = atoi(val2);
        watchdog = 0;
        break;
      }
    }
  }
#endif

#if defined(WSS_ENABLED) && defined(TICKER_ENABLED)
  void updateOutputs(){ 
    watchdog++;
    if(watchdog >= MAX_WATCHDOG){ // If no input, stop rover
      spL = 1023;
      spR = 1023;
      watchdog = 0;
    }
  
    // Smooth acceleration
    pwrL = (spL + INERT*pwrL) / (INERT+1);
    pwrR = (spR + INERT*pwrR) / (INERT+1);
  
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
  
    // Send comands as feedback
    if(wss_connected){
      char payload[8];
      sprintf(payload, "%04d%04d", pwrL, pwrR);
      webSocketSvr->sendTXT(client_num, payload); // Send values to client
    }
  
    Serial.printf("(%d,%d) \n",pwrL, pwrR);
  }
#endif

void setup() {
  Serial.begin(BAUDRATE);
  Serial.println();

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
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  config.jpeg_quality = 10;
  config.fb_count = 2;
  config.grab_mode = CAMERA_GRAB_LATEST;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  setupLedFlash(LED_GPIO_NUM);

  pinMode(FL_PIN,OUTPUT);
  pinMode(FR_PIN,OUTPUT);
  pinMode(BL_PIN,OUTPUT);
  pinMode(BR_PIN,OUTPUT);

#ifdef DISPLAY_ENABLED
  Wire.begin(SDA, SCL);
  lcd = new LiquidCrystal_I2C(LCD_ADDR, LCD_COLS, LCD_ROWS);
#endif

#if defined(WSS_ENABLED) && defined(TICKER_ENABLED)
  Ticker ticker;
  ticker.attach_ms(T_PERIOD, updateOutputs);
#endif

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  IPAddress ip = WiFi.localIP();
  int port; startCameraServer(port); // Get streaming port
  Serial.printf("Camera stream URL: http://");
  Serial.printf("%d.%d.%d.%d:%d/stream \n", ip[0], ip[1], ip[2], ip[3], port); 

#ifdef WSS_ENABLED
  port++;
  webSocketSvr = new WebSocketsServer(port);
  webSocketSvr->begin();
  webSocketSvr->onEvent(webSocketEvent);
  Serial.printf("Web Socket Server listening on http://%d.%d.%d.%d:%d\n", ip[0], ip[1], ip[2], ip[3], port);
#endif
}

void loop() {
#ifdef WSS_ENABLED
  if(!wss_connected)
    webSocketSvr->loop();
#endif
}