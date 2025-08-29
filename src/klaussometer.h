#ifndef CONSTANTS_H
#define CONSTANTS_H
#include <cstdint>
#include <time.h>

#define STORED_READING 6
#define READINGS_ARRAY                                                                                                             \
    {"Cave", "cave/tempset-ambient/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_TEMPERATURE, 0, 0},                  \
        {"Living room", "livingroom/tempset-ambient/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_TEMPERATURE, 0, 0}, \
        {"Playroom", "guest/tempset-ambient/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_TEMPERATURE, 0, 0},         \
        {"Bedroom", "bedroom/tempset-ambient/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_TEMPERATURE, 0, 0},        \
        {"Outside", "outside/tempset-ambient/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_TEMPERATURE, 0, 0},        \
        {"Cave", "cave/tempset-humidity/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_HUMIDITY, 0, 0},                \
        {"Living room", "livingroom/tempset-humidity/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_HUMIDITY, 0, 0},   \
        {"Playroom", "guest/tempset-humidity/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_HUMIDITY, 0, 0},           \
        {"Bedroom", "bedroom/tempset-humidity/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_HUMIDITY, 0, 0},          \
        {"Living room", "outside/tempset-humidity/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_HUMIDITY, 0, 0},      \
        {"Cave", "cave/battery/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_BATTERY, 0, 0},                          \
        {"Living room", "livingroom/battery/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_BATTERY, 0, 0},             \
        {"Playroom", "guest/battery/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_BATTERY, 0, 0},                     \
        {"Bedroom", "bedroom/battery/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_BATTERY, 0, 0},                    \
        {"Outside", "outside/battery/set", NO_READING, 0.0, {0.0}, CHAR_NO_MESSAGE, false, DATA_BATTERY, 0, 0}

#define ROOM_NAME_LABELS {&ui_RoomName1, &ui_RoomName2, &ui_RoomName3, &ui_RoomName4, &ui_RoomName5}
#define TEMP_ARC_LABELS {&ui_TempArc1, &ui_TempArc2, &ui_TempArc3, &ui_TempArc4, &ui_TempArc5}
#define TEMP_LABELS {&ui_TempLabel1, &ui_TempLabel2, &ui_TempLabel3, &ui_TempLabel4, &ui_TempLabel5}
#define BATTERY_LABELS {&ui_BatteryLabel1, &ui_BatteryLabel2, &ui_BatteryLabel3, &ui_BatteryLabel4, &ui_BatteryLabel5}
#define DIRECTION_LABELS {&ui_Direction1, &ui_Direction2, &ui_Direction3, &ui_Direction4, &ui_Direction5}
#define HUMIDITY_LABELS {&ui_HumidLabel1, &ui_HumidLabel2, &ui_HumidLabel3, &ui_HumidLabel4, &ui_HumidLabel5}

static const char* LOG_TOPIC = "klaussometer/log";
static const char* ERROR_TOPIC = "klaussometer/error";

static const int CHAR_LEN = 255;
#define NO_READING "--"
// Character settings
static const char CHAR_UP = 'a';   // Based on epicycles font
static const char CHAR_DOWN = 'b'; // Based on epicycles ADF font
static const char CHAR_SAME = ' '; // Based on epicycles ADF font as blank if no change
static const char CHAR_BLANK = 32;
static const char CHAR_NO_MESSAGE = '#';       // Based on epicycles ADF font
static const char CHAR_BATTERY_GOOD = 'v';     // Based on battery font
static const char CHAR_BATTERY_OK = 'u';       // Based on battery font
static const char CHAR_BATTERY_BAD = 't';      // Based on battery font
static const char CHAR_BATTERY_CRITICAL = 's'; // Based on battery font

// Define boundaries for battery health
static const float BATTERY_OK = 3.75;
static const float BATTERY_BAD = 3.6;
static const float BATTERY_CRITICAL = 3.5;

// Data type definition for array
static const int DATA_TEMPERATURE = 0;
static const int DATA_HUMIDITY = 1;
static const int DATA_SETTING = 2;
static const int DATA_ONOFF = 3;
static const int DATA_BATTERY = 4;

// Define constants used
static const int MAX_NO_MESSAGE_SEC = 3600;          // Time before CHAR_NO_MESSAGE is set in seconds (long)
static const int TIME_RETRIES = 100;                   // Number of time to retry getting the time during setup
static const int WEATHER_UPDATE_INTERVAL = 300;      // Interval between weather updates
static const int UV_UPDATE_INTERVAL = 3600;          // Interval between UV updates
static const int SOLAR_CURRENT_UPDATE_INTERVAL = 60;   // Interval between solar updates
static const int SOLAR_MONTHLY_UPDATE_INTERVAL = 300;  // Interval between solar current updates
static const int SOLAR_DAILY_UPDATE_INTERVAL = 300;    // Interval between solar daily updates
static const int STATUS_MESSAGE_TIME = 3;              // Seconds an status message can be displayed
static const int MAX_SOLAR_TIME_STATUS = 24;           // Max time in hours for charge / discharge that a message will be displayed for

static const int SCREEN_W = 1024;
static const int SCREEN_H = 600;
#define TOUCH_ROTATION ROTATION_INVERTED

static const int I2C_SDA_PIN = 17;
static const int I2C_SCL_PIN = 18;
static const int TOUCH_INT = -1;
static const int TOUCH_RST = 38;
static const int TFT_BL = 10;

static const int WIFI_RETRIES = 10; // Number of times to retry the wifi before a restart

static const int PIN_SD_CMD = 11;
static const int PIN_SD_CLK = 12;
static const int PIN_SD_D0 = 13;

static const int COLOR_RED = 0xFA0000;
static const int COLOR_YELLOW = 0xF7EA48;
static const int COLOR_GREEN = 0x205602;
static const int COLOR_BLACK = 0x000000;
static const int COLOR_WHITE = 0xFFFFFF;
#endif // CONSTANTS_H