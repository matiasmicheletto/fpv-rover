#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <Ticker.h>

// Numeros de pines
#define FL_PIN 5 // Avance izq
#define FR_PIN 4 // Avance der
#define BL_PIN 0 // Retroceso izq
#define BR_PIN 2 // Retroceso der

// Parametros
#define DEBUG true // Modo Debuggeo
#define VERBOSE true // Mostrar texto en cada intercambio
#define BAUDRATE 115200 // Velocidad serial
#define T_PERIOD 100 // Periodo de actualizacion de salidas (ms)
#define INERT 4 // Factor de inercia en aceleracion
#define MAX_WATCHDOG 10 // Maxima cantidad de loops antes de poner los setpoints en 0

// Websocket puerto 81
WebSocketsServer webSocket = WebSocketsServer(81);

// Funciones periodicas
Ticker ticker;

// Autenticacion red
const char* ssid = "B93615";
const char* password = "101344997";

int spL = 1023, spR = 1023; // Setpoints izq y der respectivamente (0..2046)
int pwrL = 1023, pwrR = 1023; // Salidas izq y der respectivamente (0..2046)

int watchdog = 0; // Contador de loops sin actualizacion de setpoint

uint8_t client_num; // Un solo cliente por vez

void updateOutputs(){ 
  // Calcular y actualizar salidas
  
  watchdog++; // Contar loop
  if(watchdog >= MAX_WATCHDOG){ // Frenar en ausencia de comandos
    spL = 1023;
    spR = 1023;
  }

  // Incremento-decremento gradual
  pwrL = (spL + INERT*pwrL) / (INERT+1);
  pwrR = (spR + INERT*pwrR) / (INERT+1);

  // Actualizar salidas
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
    Serial.printf("PL: %d, PR: %d\n", pwrL, pwrR);
  #endif

  // Enviar los valores calculados como feedback  
  char payload[8];
  sprintf(payload, "%04d%04d", pwrL, pwrR); // Concatenar valores
  webSocket.sendTXT(client_num, payload); // Enviar al cliente
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
      client_num = client; // Guardar el numero de cliente para enviarle los valores calculados
      // El payload debe tener 8 digitos solamente (controlar? length==??) y van de 0 a 2046
      char val1[4] = {(char)payload[0],(char)payload[1],(char)payload[2],(char)payload[3]};
      char val2[4] = {(char)payload[4],(char)payload[5],(char)payload[6],(char)payload[7]};
      // Convertir valores a enteros y setear setpoints
      spL = atoi(val1);
      spR = atoi(val2);
      watchdog = 0; // Reiniciar watchdog
      #ifdef VERBOSE
        Serial.printf("SPL: %d, SPR: %d\n", spL, spR);
      #endif
      break;
    }
  }
}

void setup() {
  // Inicializar GPIO
  pinMode(FL_PIN,OUTPUT);
  pinMode(FR_PIN,OUTPUT);
  pinMode(BL_PIN,OUTPUT);
  pinMode(BR_PIN,OUTPUT);

  #if defined(DEBUG) || defined(VERBOSE)
    Serial.begin(BAUDRATE); // Iniciar comunicacion con PC
  #endif
  
  // Conectarse a la red WiFi
  WiFi.begin(ssid, password);  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    #ifdef DEBUG
      Serial.printf(".");
    #endif
  }
  #ifdef DEBUG // TODO mostrar esto en display 
    Serial.printf("\nWiFi conectado\n");
    Serial.print("IP: http://");
    Serial.print(WiFi.localIP()); 
    Serial.println("/");
  #endif
 
  webSocket.begin(); // Iniciar WebSocket Server
  webSocket.onEvent(webSocketEvent); // Habilitar evento

  ticker.attach_ms(T_PERIOD, updateOutputs); // Iniciar funcion periodica
}

void loop() {
  webSocket.loop(); // Solo chequea eventos y ejecuta el callback
}
