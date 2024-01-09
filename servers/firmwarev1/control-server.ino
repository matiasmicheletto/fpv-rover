#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <Ticker.h>

// Pinout ESP8266 --> L298n inputs
#define FL_PIN 5 // Left forward --> In_3
#define BL_PIN 0 // Left backward --> In_4
#define FR_PIN 4 // Right forward --> In_1
#define BR_PIN 2 // Right backward --> In_2

#define DEBUG true
//#define VERBOSE true // Print all text
#define BAUDRATE 115200
#define T_PERIOD 100 // Output updating period (ms)
#define INERT 6 // Acceleration inertia factor
#define MAX_WATCHDOG 10 // Max. number of loops before all stop
#define WIFI_PASS true // If wifi has password

WebSocketsServer webSocket = WebSocketsServer(81);

// Control of periodic functions
Ticker ticker;

// Network credentials (to setup later on mobile as access point)
const char* ssid = "EyeRobot";
#ifdef WIFI_PASS
  const char* password = "eyerobot123";
#endif

int spL = 1023, spR = 1023; // Left and right (0..2046)
int pwrL = 1023, pwrR = 1023; // Left and right outputs (0..2046)
int watchdog = 0; // Loop counter

uint8_t client_num; // Number of clients 

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

  #ifdef VERBOSE
    Serial.printf("SPL: %d, SPR: %d -- PL: %d, PR: %d\n", spL, spR, pwrL, pwrR);    
  #endif

  // Send comands as feedback
  char payload[8];
  sprintf(payload, "%04d%04d", pwrL, pwrR);
  webSocket.sendTXT(client_num, payload); // Send values to client
}

void webSocketEvent(uint8_t client, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(client);
      #ifdef DEBUG
        Serial.printf("[%u] Conectado a la URL: %d.%d.%d.%d - %s\n", client, ip[0], ip[1], ip[2], ip[3], payload); 
      #endif
      break;
    }
    case WStype_DISCONNECTED:{
      #ifdef DEBUG
        Serial.printf("[%u] Desconectado!\n", client);
      #endif
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

void setup() {
  pinMode(FL_PIN,OUTPUT);
  pinMode(FR_PIN,OUTPUT);
  pinMode(BL_PIN,OUTPUT);
  pinMode(BR_PIN,OUTPUT);

  #if defined(DEBUG) || defined(VERBOSE)
    Serial.begin(BAUDRATE);
  #endif
  
  #ifdef WIFI_PASS
    WiFi.begin(ssid, password);
  #else
    WiFi.begin(ssid); // Sin password
  #endif
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    #ifdef DEBUG
      Serial.printf(".");
    #endif
  }
  #ifdef DEBUG
    Serial.printf("\nWiFi conectado\n");
    Serial.print("IP: http://");
    Serial.print(WiFi.localIP()); 
    Serial.println("/");
  #endif
 
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  ticker.attach_ms(T_PERIOD, updateOutputs);
}

void loop() {
  webSocket.loop();
}
