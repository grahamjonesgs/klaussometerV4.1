// 
#include <time.h>
#define STORED_READING 6
#define READINGS_ARRAY  \
{"Cave", "cave/tempset-ambient/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_TEMPERATURE, 0, 0}, \
  {"Living room", "livingroom/tempset-ambient/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_TEMPERATURE, 0,0},\
  {"Playroom", "guest/tempset-ambient/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_TEMPERATURE, 0,0}, \
  {"Bedroom", "bedroom/tempset-ambient/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_TEMPERATURE, 0,0}, \
  {"Outside", "outside/tempset-ambient/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_TEMPERATURE, 0,0}, \
  {"Cave", "cave/tempset-humidity/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_HUMIDITY, 0,0}, \
  {"Living room", "livingroom/tempset-humidity/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_HUMIDITY, 0,0},\
  {"Playroom", "guest/tempset-humidity/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_HUMIDITY, 0,0}, \
  {"Bedroom", "bedroom/tempset-humidity/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_HUMIDITY, 0,0}, \
  {"Outside", "outside/tempset-humidity/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_HUMIDITY, 0,0}, \
  {"Outside", "outside/battery/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_BATTERY, 0,0} 

#define CHAR_LEN 255

#include <cstdint>


#define NO_READING "--"
// Character settings
#define CHAR_UP 'a'    // Based on epicycles font
#define CHAR_DOWN 'b'  // Based on epicycles ADF font
#define CHAR_SAME ' '  // Based on epicycles ADF font as blank if no change
#define CHAR_BLANK 32
#define CHAR_NO_MESSAGE '#'  // Based on epicycles ADF font
#define CHAR_BATTERY_GOOD 'v' // Based on battery font
#define CHAR_BATTERY_OK 'u' // Based on battery font
#define CHAR_BATTERY_BAD 't' // Based on battery font
#define CHAR_BATTERY_CRITICAL 's' // Based on battery font

// Define boundaries for battery health
#define BATTERY_OK 3.75
#define BATTERY_BAD 3.6
#define BATTERY_CRITICAL 3.5

// Data type definition for array
#define DATA_TEMPERATURE 0
#define DATA_HUMIDITY 1
#define DATA_SETTING 2
#define DATA_ONOFF 3
#define DATA_BATTERY 4

// Define constants used
#define MAX_NO_MESSAGE_SEC 3600LL            // Time before CHAR_NO_MESSAGE is set in seconds (long)
#define TIME_RETRIES 100                     // Number of time to retry getting the time during setup
#define WEATHER_UPDATE_INTERVAL 300LL          // Interval between weather updates
#define UV_UPDATE_INTERVAL 3600LL            // Interval between UV updates (long)
#define SOLAR_UPDATE_INTERVAL 30             // Interval between solar updates
#define STATUS_MESSAGE_TIME 5                // Seconds an status message can be displayed
#define MAX_SOLAR_TIME_STATUS 24             // Max time in hours for charge / discharge that a message will be displayed for

#define SCREEN_W 1024
#define SCREEN_H 600
#define TOUCH_ROTATION ROTATION_INVERTED

#define I2C_SDA_PIN 17
#define I2C_SCL_PIN 18
#define TOUCH_INT -1
#define TOUCH_RST 38
#define TFT_BL 10

#define WIFI_RETRIES 100  // Number of times to retry the wifi before a restart
#define CHAR_LEN 255

#define PIN_SD_CMD 11
#define PIN_SD_CLK 12
#define PIN_SD_D0 13

#define COLOR_RED 0xFA0000
#define COLOR_YELLOW 0xF7EA48
#define COLOR_GREEN 0x205602
#define COLOR_BLACK 0x000000
#define COLOR_WHITE 0xFFFFFF
