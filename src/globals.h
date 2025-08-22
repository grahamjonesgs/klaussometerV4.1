#include <lvgl.h>  // Version 8.4 tested
#include "UI/ui.h"
#include <Arduino_GFX_Library.h>
#include <TAMC_GT911.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoMqttClient.h>
#include <HTTPClient.h>
#include "klaussometer.h"
#include "wifi_user.h"
#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <Preferences.h>

struct Readings {                   // Array to hold the incoming measurement
  const char description[50];       // Currently set to 50 chars long
  const char topic[50];             // MQTT topic
  char output[10];                  // To be output to screen
  float currentValue;               // Current value received
  float lastValue[STORED_READING];  // Defined that the zeroth element is the oldest
  uint8_t changeChar;               // To indicate change in status
  bool enoughData;                  // to indicate is a full set of STORED_READING number of data points received
  int dataType;                     // Type of data received
  int readingIndex;                 // Index of current reading max will be STORED_READING
  unsigned long lastMessageTime;    // Millis this was last updated
};

struct Weather {
  float temperature;
  float windSpeed;
  float UV;
  float maxTemp;
  float minTemp;
  bool isDay;
  time_t updateTime;
  time_t UVupdateTime;
  char windDir[CHAR_LEN];
  char description[CHAR_LEN];
  char weather_time_string[CHAR_LEN];
  char UV_time_string[CHAR_LEN];
};

struct Solar {
  time_t updateTime;
  float batteryCharge;
  float usingPower;
  float gridPower;
  float batteryPower;
  float solarPower;
  char  time[CHAR_LEN];
  float today_battery_min;
  float today_battery_max;
  bool minmax_reset;
  float today_buy;
  float month_buy;
};

typedef struct {
    char text[CHAR_LEN];
    int duration_s; // Duration in seconds
} StatusMessage;

//main
void pin_init();
void setup_wifi();
void mqtt_connect(); 
void touch_init();
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
void setup_wifi();
void mqtt_connect();
void time_init();
void receive_mqtt_messages_t(void *pvParams);
void touch_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
void set_solar_values();
void getBatteryStatus(float batteryValue, int readingIndex,
                      char *iconCharacterPtr, lv_color_t *colorPtr);
void displayStatusMessages_t(void *pvParameters);
void logAndPublish(const char* format, ...);
void errorPublish(const char* format, ...);

// mqtt
void update_temperature(char *recMessage, int index);
void update_humidity(char *recMessage, int index);
void update_battery(char *recMessage, int index);
void update_battery(char *recMessage, int index);

//  Screen updates
void set_temperature_values();
void set_humidity_values();
void set_battery_values();
int uv_color(float UV);
char *format_integer_with_commas(long long num);
void set_basic_text_color(lv_color_t color);


// APIs
void get_uv_t(void *pvParameters);
void get_weather_t(void *pvParameters);
void get_solar_t(void *pvParameters);
const char *degreesToDirection(double degrees);
const char *wmoToText(int code, bool isDay);
