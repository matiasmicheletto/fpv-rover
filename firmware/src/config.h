// Misc
#define BAUDRATE 115200 // Serial communication speed
#define RS232_DEBUG 1 // Print debug messages
#define INERT 0.3 // Acceleration inertia factor
#define MAX_WATCHDOG 10 // Max. number of loops before all stop
#define INT_EPS 1 // Epsilon for integer comparison
#define PWM_MAX 255 // Max. PWM value (8 bits)
#define CLAMP_THRES 100 // Clamp threshold for motor pwm outputs

#define WIFI_CONNECT_ATTEMPTS 50 // Max number of attempts to connect to WiFi

// Server ports
// Make sure to change the ports in the client's code
#define CAM_PORT 80 
#define WSS_PORT 82

// OV260 pinout
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
#define LED_GPIO_NUM      33
#define FLASH_LED_NUM     4

// GPIO
#define FL_PIN 13 // Left forward --> In_3
#define BL_PIN 14 // Left backward --> In_4
#define FR_PIN 15 // Right forward --> In_1
#define BR_PIN 2 // Right backward --> In_2


/* Not using LCD for now
#define LCD_ADDR 0x27 // Or 0x3F if not working
#define LCD_COLS 20
#define LCD_ROWS 4
*/