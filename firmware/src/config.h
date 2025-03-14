// General config
#define BAUDRATE 115200
#define RS232_DEBUG 1 // Print setpoint and outputs
#define INERT 0.1 // Acceleration inertia factor
#define MAX_WATCHDOG 10 // Max. number of loops before all stop
#define PAYLOAD_DIGITS 6 // Number of digits in payload
#define INT_EPS 1 // Epsilon for integer comparison
#define PWM_MAX 255 // Max. PWM value (8 bits)

// Libs isolation
#define WIFI_ENABLED 1
#define CAM_ENABLED 1
#define WSS_ENABLED 1
#define PWM_OUTPUTS_ENABLED 1
//#define DISPLAY_ENABLED 1

#ifdef WIFI_ENABLED
  #include <WiFi.h>
  #include "credentials.h"
  #define WIFI_CONNECT_ATTEMPTS 20 // Max number of attempts to connect to WiFi
#endif
#ifdef CAM_ENABLED
  #include "esp_camera.h"
  #define CAM_PORT 80
#endif
#ifdef WSS_ENABLED
  #include "WebSocketsServer.h"
  #define WSS_PORT 82
#endif
#ifdef DISPLAY_ENABLED
  #include "LiquidCrystal_I2C.h"
#endif

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
