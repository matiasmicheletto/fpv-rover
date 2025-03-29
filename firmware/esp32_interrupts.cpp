#include "WiFi.h"
#include "esp_camera.h"
#include "WebSocketsServer.h"
#include "esp_timer.h"

volatile uint32_t timertick = 0;
hw_timer_t * timer = NULL;


/*
int parseValue(char * ptr, int size) { // Refactored this function
  int value = 0;
  for (int i = 0; i < size; i++) {
    if (ptr[i] < '0' || ptr[i] > '9') return 0; // Error handling
    value = value * 10 + (ptr[i] - '0'); // Convert char to int
  }
  return value;
}
*/

void IRAM_ATTR onTimer() {
  timertick++;
}

void functionWebSocket() {
  webSocketSvr.loop();  // Periodic WebSocket processing
}

void functionA() {
  Serial.println("Function A executed (100ms)");
}

void functionB() {
  Serial.println("Function B executed (200ms)");
}

void functionC() {
  Serial.println("Function C executed (500ms)");
}

void setup() {
  Serial.begin(115200);

  // Configure Timer 2 (Timer Group 1, Timer 2)
  timer = timerBegin(2, 80, true);  // Timer 2, prescaler 80 -> 1 tick = 1μs
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 10000, true); // Interrupt every 10ms (10000µs)
  timerAlarmEnable(timer); // Enable timer
}

void loop() {
  switch (timertick) {
    case 5: // 50ms 
      functionWebSocket();
    case 10: // 100 ms
      functionA();
      break;
    case 20: // 200 ms
      functionB();
      break;
    case 50: // 500 ms
      functionC();
      timertick = 0; // Reset counter
      break;
    default:
      break;
  }
}