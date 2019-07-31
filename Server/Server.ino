#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>

const char* ssid = "B93615";
const char* password = "101344997";

WebSocketsServer webSocket = WebSocketsServer(81);

void webSocketEvent(uint8_t client_num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(client_num);
      Serial.printf("[%u] Conectado a la URL: %d.%d.%d.%d - %s\n", client_num, ip[0], ip[1], ip[2], ip[3], payload); 
      break;
    }
    case WStype_DISCONNECTED:{
      Serial.printf("[%u] Desconectado!\n", client_num);
      break;
    }
    case WStype_TEXT:{
      //Serial.printf("Número de conexión: %u  -  Carácteres recibidos: %s\n  ", client_num, payload);
      webSocket.sendTXT(client_num, payload); // Reenviar el mensaje como ack
      //char data[6]; data = (char*)payload; // Conversión de los carácteres recibidos a cadena de texto
      char val1[3] = {(char)payload[0],(char)payload[1],(char)payload[2]};
      char val2[3] = {(char)payload[3],(char)payload[4],(char)payload[5]};
      // Configurar salidas
      setOutputs(atoi(val1), atoi(val2));
      break;
    }
  }
}

void setOutputs(int val1, int val2) {
  Serial.printf("Motor 1: %d, Motor 2: %d\n",val1, val2);
}

void setup() {
  Serial.begin(115200); // Iniciar comunicacion con PC

  WiFi.begin(ssid, password); // Conectarse a la red WiFi 
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.printf(".");
  }

  Serial.printf("\nWiFi conectado\n");
  Serial.print("Esta es mi IP: http://");
  Serial.print(WiFi.localIP()); 
  Serial.println("/");
 
  webSocket.begin(); // Iniciar WebSocket Server
  webSocket.onEvent(webSocketEvent); // Habilitar evento
}

void loop() {
  webSocket.loop();
}
