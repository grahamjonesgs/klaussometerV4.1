/* Set ESP32 type to ESP-S3 Dev Module
 lvgl 8.3.6, GFX Lib Arduino 1.50, TAMC GT911 1.0.2, Squareline 1.4.0 and
esp32 3.3.

n Arduino IDE Tools menu:
Board: ESP32 Dev Module
Flash Size: 16M Flash
Flash Mode QIO 80Mhz
Partition Scheme: 16MB (2MB App/...)
PSRAM: OPI PSRAM
Events Core 1
Arduino Core 0
*/

#include "klaussometer.h"
#include "time.h"
#include "ui.h"
#include "wifi_user.h"
#include <ArduinoJson.h>
#include <ArduinoMqttClient.h>
#include <Arduino_GFX_Library.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <Preferences.h>
#include <TAMC_GT911.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <globals.h>
#include <klaussometer.h>
#include <lvgl.h> // Version 8.4 tested
#include "FreeRTOS.h" // Include FreeRTOS for queue functionality
#include "queue.h"    // Queue-specific functions
#include "task.h"     // Task-specific functions
#include <lvgl.h>     // For LVGL UI functions
#include <stdarg.h>   // For va_list, va_start, va_end

// Create network objects
WiFiClient espClient;
MqttClient mqttClient(espClient);
WiFiUDP ntpUDP;
SemaphoreHandle_t mqttMutex;

// Global variables
time_t statusChangeTime = 0;
NTPClient timeClient(ntpUDP, "pool.ntp.org", TIME_OFFSET, 60000);
void touch_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
Weather weather = {0.0, 0.0, 0.0, 0.0, 0.0,        false,
                   0,   0,   "",  "",  "--:--:--", "--:--:--"};
Solar solar = {0, 0.0, 0.0, 0.0, 0.0, 0.0, "--:--:--", 100, 0, false, 0.0, 0.0};
Readings readings[]{READINGS_ARRAY};
Preferences storage;
int numberOfReadings = sizeof(readings) / sizeof(readings[0]);
QueueHandle_t statusMessageQueue;

// Status messages
char statusMessage[CHAR_LEN];
bool statusMessageUpdated = false;

// For Backlight PWM
const int PWMFreq = 5000;
const int PWMChannel = 4;
const int PWMResolution = 10;
const int MAX_DUTY_CYCLE = (int)(pow(2, PWMResolution) - 1);
const int day_duty = 0;
const int night_duty = MAX_DUTY_CYCLE * 0.97;

// Screen config
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    40 /* DE */, 41 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */, 45 /* R0 */,
    48 /* R1 */, 47 /* R2 */, 21 /* R3 */, 14 /* R4 */, 5 /* G0 */, 6 /* G1 */,
    7 /* G2 */, 15 /* G3 */, 16 /* G4 */, 4 /* G5 */, 8 /* B0 */, 3 /* B1 */,
    46 /* B2 */, 9 /* B3 */, 1 /* B4 */, 0 /* hsync_polarity */,
    40 /* hsync_front_porch */, 8 /* hsync_pulse_width was 48 */,
    128 /* hsync_back_porch */, 1 /* vsync_polarity */,
    13 /* vsync_front_porch */, 8 /* vsync_pulse_width was 3 */,
    45 /* vsync_back_porch */, 1 /* pclk_active_neg */,
    12000000 /*was 16000000*/ /* prefer_speed */);
Arduino_RGB_Display *gfx_new =
    new Arduino_RGB_Display(1024 /* width */, 600 /* height */, rgbpanel);

// Touch config
TAMC_GT911 ts =
    TAMC_GT911(I2C_SDA_PIN, I2C_SCL_PIN, TOUCH_INT, TOUCH_RST, 1024, 600);
int touch_last_x = 0;
int touch_last_y = 0;

// Screen setting
static uint32_t screenWidth = SCREEN_W;
static uint32_t screenHeight = SCREEN_H;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;
static lv_disp_drv_t disp_drv;

// Arrays of UI objects
#define ROOM_COUNT 5
static lv_obj_t **roomNames[ROOM_COUNT] = ROOM_NAME_LABELS;
static lv_obj_t **tempArcs[ROOM_COUNT] = TEMP_ARC_LABELS;
static lv_obj_t **tempLabels[ROOM_COUNT] = TEMP_LABELS;
static lv_obj_t **batteryLabels[ROOM_COUNT] = BATTERY_LABELS;
static lv_obj_t **directionLabels[ROOM_COUNT] = DIRECTION_LABELS;
static lv_obj_t **humidityLabels[ROOM_COUNT] = HUMIDITY_LABELS;


void setup() {
  Serial.begin(115200);
  Serial.println("Starting Klaussometer 4.0 Display");

  statusMessageQueue = xQueueCreate(100, sizeof(StatusMessage));
  mqttMutex = xSemaphoreCreateMutex();
    
    // Check if the queue was created successfully
    if (statusMessageQueue == NULL) {
        // Handle error: The queue could not be created
        Serial.println("Error: Failed to create status message queue.");
    }

  pin_init();
  touch_init();

  // Init Display
  gfx_new->begin();
  gfx_new->fillScreen(BLACK);
  lv_init();
  screenWidth = gfx_new->width();
  screenHeight = gfx_new->height();
  disp_draw_buf =
      (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * screenWidth * 10,
                                     MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

  // Create display buffer
  if (!disp_draw_buf) {
    Serial.println("LVGL disp_draw_buf allocate failed!");
  } else {
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, screenWidth * 10);

    /* Initialize the display */
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /* Initialize the input device driver */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read;
    lv_indev_drv_register(&indev_drv);

    ui_init();
    lv_timer_handler();

    // Set the initial values

    for (unsigned char i = 0; i < ROOM_COUNT; ++i) {
      lv_label_set_text(*roomNames[i], readings[i].description);
      lv_arc_set_value(*tempArcs[i], readings[i].currentValue);
      lv_obj_add_flag(*tempArcs[i], LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(*tempLabels[i], readings[i].output);
      lv_label_set_text(*directionLabels[i], "");
      lv_label_set_text(*humidityLabels[i], readings[i + ROOM_COUNT].output);
      lv_label_set_text(*batteryLabels[i], "");
    }

    lv_label_set_text(ui_FCConditions, "Pending");
    lv_label_set_text(ui_FCWindSpeed, "Pending");
    lv_label_set_text(ui_FCUpdateTime, "Pending");
    lv_label_set_text(ui_UVUpdateTime, "Pending");
    lv_label_set_text(ui_TempLabelFC, "--");
    lv_label_set_text(ui_UVLabel, "--");

    lv_arc_set_value(ui_BatteryArc, 0);
    lv_label_set_text(ui_BatteryLabel, "--");
    lv_arc_set_value(ui_SolarArc, 0);
    lv_label_set_text(ui_SolarLabel, "--");
    lv_arc_set_value(ui_UsingArc, 0);
    lv_label_set_text(ui_UsingLabel, "--");
    lv_label_set_text(ui_ChargingLabel, "");
    lv_label_set_text(ui_AsofTimeLabel, "Values as of --:--:--");
    lv_label_set_text(ui_ChargingTime, "");
    lv_label_set_text(ui_SolarMinMax, "");

    lv_obj_set_style_text_color(ui_WiFiStatus, lv_color_hex(COLOR_RED),
                                LV_PART_MAIN);
    lv_obj_set_style_text_color(ui_ServerStatus, lv_color_hex(COLOR_RED),
                                LV_PART_MAIN);
    lv_obj_set_style_text_color(ui_WeatherStatus, lv_color_hex(COLOR_RED),
                                LV_PART_MAIN);
    lv_obj_set_style_text_color(ui_SolarStatus, lv_color_hex(COLOR_RED),
                                LV_PART_MAIN);

    // Set to night settings at first
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(COLOR_BLACK),
                              LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_Container1, lv_color_hex(COLOR_WHITE),
                                  LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_Container2, lv_color_hex(COLOR_WHITE),
                                  LV_STATE_DEFAULT);

    lv_label_set_text(ui_GridBought,
                      "Bought\nToday - Pending\nThis Month - Pending");

    logAndPublish("Starting Wifi");
    setup_wifi();
    logAndPublish("Getting time");
    time_init();
    pin_init();
    touch_init();
    logAndPublish("Connecting to MQTT server");
    mqtt_connect();
    logAndPublish("MQTT server connected");


    // Start tasks
    xTaskCreatePinnedToCore(receive_mqtt_messages_t, "mqtt", 16384, NULL, 4,
                            NULL, 0);
    xTaskCreatePinnedToCore(get_weather_t, "Get Weather", 8192, NULL, 3, NULL,
                            0);
    xTaskCreatePinnedToCore(get_uv_t, "Get UV", 8192, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(get_solar_t, "Get Solar", 8192, NULL, 3, NULL, 0);
    xTaskCreate(displayStatusMessages_t, "DisplayStatus", 4096, NULL, 0, NULL);
  }
}

void loop() {
  char tempString[CHAR_LEN];
  char timeString[CHAR_LEN];
  char icon;
  lv_color_t color;

  delay(50);
  lv_timer_handler(); // Run GUI

  // Update values

  for (unsigned char i = 0; i < ROOM_COUNT; ++i) {
    lv_arc_set_value(*tempArcs[i], readings[i].currentValue);
    lv_label_set_text(*tempLabels[i], readings[i].output);
    if (readings[i].changeChar != CHAR_NO_MESSAGE) {
      lv_obj_clear_flag(*tempArcs[i], LV_OBJ_FLAG_HIDDEN);
    }
    snprintf(tempString, CHAR_LEN, "%c", readings[i].changeChar);
    lv_label_set_text(*directionLabels[i], tempString);
    lv_label_set_text(*humidityLabels[i], readings[i + ROOM_COUNT].output);
  }

  // Battery updates

  for (unsigned char i = 0; i < ROOM_COUNT; ++i) {
    getBatteryStatus(readings[i + 2 * ROOM_COUNT].currentValue,
                     readings[i + 2 * ROOM_COUNT].readingIndex, &icon, &color);
    snprintf(tempString, CHAR_LEN, "%c", icon);
    lv_label_set_text(*batteryLabels[i], tempString);
    lv_obj_set_style_text_color(*batteryLabels[i], color, LV_PART_MAIN);
  }

  // Update UV
  if (weather.UVupdateTime > 0) {
    snprintf(tempString, CHAR_LEN, "Updated %s", weather.UV_time_string);
    lv_label_set_text(ui_UVUpdateTime, tempString);
    snprintf(tempString, CHAR_LEN, "%2.1f", weather.UV);
    lv_label_set_text(ui_UVLabel, tempString);
    lv_arc_set_value(ui_UVArc, weather.UV * 10);

    lv_obj_set_style_arc_color(ui_UVArc, lv_color_hex(uv_color(weather.UV)),
                               LV_PART_INDICATOR |
                                   LV_STATE_DEFAULT); // Set arc to color
    lv_obj_set_style_bg_color(ui_UVArc, lv_color_hex(uv_color(weather.UV)),
                              LV_PART_KNOB |
                                  LV_STATE_DEFAULT); // Set arc to color
  }

  // Update weather values
  if (weather.updateTime > 0) {
    lv_label_set_text(ui_FCConditions, weather.description);
    snprintf(tempString, CHAR_LEN, "Updated %s", weather.weather_time_string);
    lv_label_set_text(ui_FCUpdateTime, tempString);
    snprintf(tempString, CHAR_LEN, "Wind %2.0f km/h %s", weather.windSpeed,
             weather.windDir);
    lv_label_set_text(ui_FCWindSpeed, tempString);

    lv_arc_set_value(ui_TempArcFC, weather.temperature);

    snprintf(tempString, CHAR_LEN, "%2.1f", weather.temperature);
    lv_label_set_text(ui_TempLabelFC, tempString);

    // Set min max if outside the expected values
    if (weather.temperature < weather.minTemp) {
      weather.minTemp = weather.temperature;
    }
    if (weather.temperature > weather.maxTemp) {
      weather.maxTemp = weather.temperature;
    }
  }

  if (weather.updateTime > 0) {
    snprintf(tempString, CHAR_LEN, "Min\n%2.0fC", weather.minTemp);
    lv_label_set_text(ui_FCMin, tempString);
    snprintf(tempString, CHAR_LEN, "Max\n%2.0fC", weather.maxTemp);
    lv_label_set_text(ui_FCMax, tempString);
    lv_arc_set_range(ui_TempArcFC, weather.minTemp, weather.maxTemp);
  }

  // Update solar values
  set_solar_values();
  if (now() - solar.updateTime > 2 * SOLAR_UPDATE_INTERVAL) {
    lv_obj_set_style_text_color(ui_SolarStatus, lv_color_hex(COLOR_RED),
                                LV_PART_MAIN);
  } else {
    lv_obj_set_style_text_color(ui_SolarStatus, lv_color_hex(COLOR_GREEN),
                                LV_PART_MAIN);
  }
  if (now() - weather.updateTime > 2 * WEATHER_UPDATE_INTERVAL) {
    lv_obj_set_style_text_color(ui_WeatherStatus, lv_color_hex(COLOR_RED),
                                LV_PART_MAIN);
  } else {
    lv_obj_set_style_text_color(ui_WeatherStatus, lv_color_hex(COLOR_GREEN),
                                LV_PART_MAIN);
  }

  if (WiFi.status() != WL_CONNECTED) {
    lv_obj_set_style_text_color(ui_WiFiStatus, lv_color_hex(COLOR_RED),
                                LV_PART_MAIN);
  } else {
    lv_obj_set_style_text_color(ui_WiFiStatus, lv_color_hex(COLOR_GREEN),
                                LV_PART_MAIN);
  }

  // Remove old status messages
  /* if (statusChangeTime + STATUS_MESSAGE_TIME < now()) {
    lv_label_set_text(ui_StatusMessage, "");
  }

  // Display new status message
  if (statusMessageUpdated) {
    statusMessageUpdated = false;
    lv_label_set_text(ui_StatusMessage, statusMessage);
    statusChangeTime = now();
  } */

  // Update time and screen brightness
  timeClient.getFormattedTime().toCharArray(timeString, CHAR_LEN);
  lv_label_set_text(ui_Time, timeString);

  if (!weather.isDay) {
    ledcWrite(PWMChannel, night_duty);
    set_basic_text_color(lv_color_hex(COLOR_WHITE));
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(COLOR_BLACK),
                              LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_Container1, lv_color_hex(COLOR_WHITE),
                                  LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_Container2, lv_color_hex(COLOR_WHITE),
                                  LV_STATE_DEFAULT);

  } else {
    ledcWrite(PWMChannel, day_duty);
    set_basic_text_color(lv_color_hex(COLOR_BLACK));
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(COLOR_WHITE),
                              LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_Container1, lv_color_hex(COLOR_BLACK),
                                  LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_Container2, lv_color_hex(COLOR_BLACK),
                                  LV_STATE_DEFAULT);
  }

  // Invalidate readings if too old
  for (int i = 0; i < sizeof(readings) / sizeof(readings[0]); i++) {
    if ((millis() >
         readings[i].lastMessageTime + (MAX_NO_MESSAGE_SEC * 1000)) &&
        (strcmp(readings[i].output, NO_READING) != 0) &&
        (readings[i].changeChar != CHAR_NO_MESSAGE)) {
      readings[i].changeChar = CHAR_NO_MESSAGE;
      snprintf(readings[i].output, 10, NO_READING);
      readings[i].currentValue = 0.0;
    }
  }
}

// Flush function for LVGL
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area,
                   lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
  gfx_new->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full,
                                w, h);
#else
  gfx_new->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w,
                              h);
#endif

  lv_disp_flush_ready(disp);
}

// Initialise pins for touch and backlight
void pin_init() {
  pinMode(TFT_BL, OUTPUT);
  pinMode(TOUCH_RST, OUTPUT);

  //(Replaced with ledcAttachChannel in ESP 3.0)
  ledcSetup(PWMChannel, PWMFreq, PWMResolution);
  ledcAttachPin(TFT_BL, PWMChannel);

  /*ledcAttachChannel(TFT_BL, PWMFreq, PWMResolution, PWMChannel); */

  ledcWrite(PWMChannel, night_duty); // Start dim

  delay(100);
  digitalWrite(TOUCH_RST, LOW);
  delay(1000);
  digitalWrite(TOUCH_RST, HIGH);
  delay(1000);
  digitalWrite(TOUCH_RST, LOW);
  delay(1000);
  digitalWrite(TOUCH_RST, HIGH);
  delay(1000);
}

// Initialise touch screen
void touch_init(void) {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  ts.begin();
  ts.setRotation(TOUCH_ROTATION);
}

void touch_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  ts.read();
  if (ts.isTouched) {
    touch_last_x = map(ts.points[0].x, 0, 1024, 0, SCREEN_W);
    touch_last_y = map(ts.points[0].y, 0, 750, 0, SCREEN_H);
    data->point.x = touch_last_x;
    data->point.x = touch_last_x;
    data->state = LV_INDEV_STATE_PR;

    ts.isTouched = false;
  } else {
    data->point.x = touch_last_x;
    data->point.x = touch_last_x;
    data->state = LV_INDEV_STATE_REL;
  }
}

void getBatteryStatus(float batteryValue, int readingIndex,
                      char *iconCharacterPtr, lv_color_t *colorPtr) {
  // Use a single if-else if-else structure for cleaner logic
  if (batteryValue > BATTERY_BAD) {
    // Battery is good
    *iconCharacterPtr = CHAR_BATTERY_OK;
    *colorPtr = lv_color_hex(COLOR_GREEN);
  } else if (batteryValue > BATTERY_CRITICAL) {
    // Battery is low, but not critical
    *iconCharacterPtr = CHAR_BATTERY_BAD;
    *colorPtr = lv_color_hex(COLOR_YELLOW);
  } else {
    // Battery is critical
    if (readingIndex != 0) {
      *iconCharacterPtr = CHAR_BATTERY_CRITICAL;
      *colorPtr = lv_color_hex(COLOR_RED);
    } else {
      // Default or fallback case if readingIndex is 0
      *iconCharacterPtr = CHAR_BLANK; // Or some other default
      *colorPtr = lv_color_hex(COLOR_GREEN);
    }
  }
}

void displayStatusMessages_t(void *pvParameters) {
    StatusMessage receivedMsg;

    while (true) {
        // Wait indefinitely for a new message to arrive in the queue.
        // portMAX_DELAY ensures the task will sleep until a message is available.
        if (xQueueReceive(statusMessageQueue, &receivedMsg, portMAX_DELAY) == pdTRUE) {
            // A message was received, so update the LVGL label.
            lv_label_set_text(ui_StatusMessage, receivedMsg.text);

            lv_task_handler();

            // Wait for the specified duration before clearing the message.
            vTaskDelay(pdMS_TO_TICKS(receivedMsg.duration_s * 1000));

            // Clear the label after the duration has passed.
            lv_label_set_text(ui_StatusMessage, "");
            lv_task_handler(); // Flush the display again
        }
    }
}


void logAndPublish(const char* format, ...) {
    char messageBuffer[CHAR_LEN];
    va_list args;
    
    // Format the message using the variable arguments
    va_start(args, format);
    vsnprintf(messageBuffer, sizeof(messageBuffer), format, args);
    va_end(args);

    // Print to the serial console
    Serial.println(messageBuffer);

    // Check if the MQTT client is connected and publish the message
    if (xSemaphoreTake(mqttMutex, portMAX_DELAY) == pdTRUE) {
        // We have successfully acquired the lock
        if (mqttClient.connected()) {
            mqttClient.beginMessage(LOG_TOPIC);
            mqttClient.print(messageBuffer);
            mqttClient.endMessage();
        }
        // Give the mutex back to allow other tasks to use the client
        xSemaphoreGive(mqttMutex);
    } 

    // Also send the message to the UI status queue for on-screen display.
    StatusMessage msg;
    strncpy(msg.text, messageBuffer, CHAR_LEN);
    msg.duration_s = STATUS_MESSAGE_TIME;
    xQueueSend(statusMessageQueue, &msg, 0); // Use 0 for no-wait if queue is full
}