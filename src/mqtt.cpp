#include <globals.h>
#include <klaussometer.h>

extern MqttClient mqttClient;
extern char statusMessage[];
extern bool statusMessageUpdated;
extern Readings readings[];
extern int numberOfReadings;

// Get mqtt messages
void receive_mqtt_messages_t(void *pvParams) {
  int messageSize = 0;
  String topic;
  char recMessage[CHAR_LEN] = { 0 };
  int index;
  while (true) {
    delay(100);
    if (WiFi.status() != WL_CONNECTED) {
      setup_wifi();
    }
    if (!mqttClient.connected()) {
      mqtt_connect();
    } else {
      if (lv_is_initialized()) {
        lv_obj_set_style_text_color(ui_ServerStatus, lv_color_hex(COLOR_GREEN), LV_PART_MAIN);
      }
    }
    delay(100);
    messageSize = mqttClient.parseMessage();
    if (messageSize) {  // Message received
      topic = mqttClient.messageTopic();
      mqttClient.read((unsigned char *)recMessage, (size_t)sizeof(recMessage));  // Destructive read of message
      recMessage[messageSize] = 0;
      //Serial.println("Topic: " + String(topic) + " Msg: " + recMessage);

      for (int i = 0; i < numberOfReadings ; i++) {
        if (topic == String(readings[i].topic)) {
          index = i;
          if (readings[i].dataType == DATA_TEMPERATURE) {
            update_temperature(recMessage, index);
          }
          if (readings[i].dataType == DATA_HUMIDITY) {
            update_humidity(recMessage, index);
          }
          if (readings[i].dataType == DATA_BATTERY) {
            update_battery(recMessage, index);
          }
        }
      }
    }
  }
}

// Update temperature settings
void update_temperature(char *recMessage, int index) {

  float averageHistory;
  float totalHistory = 0.0;

  readings[index].currentValue = atof(recMessage);
  snprintf(readings[index].output, 10, "%2.1f", readings[index].currentValue);

  if (readings[index].readingIndex == 0) {
    readings[index].changeChar = CHAR_BLANK;  // First reading of this boot
    readings[index].lastValue[0] = readings[index].currentValue;
  } else {
    for (int i = 0; i < readings[index].readingIndex; i++) {
      totalHistory = totalHistory + readings[index].lastValue[i];
    }
    averageHistory = totalHistory / readings[index].readingIndex;

    if (readings[index].currentValue > averageHistory) {
      readings[index].changeChar = CHAR_UP;
    }
    if (readings[index].currentValue < averageHistory) {
      readings[index].changeChar = CHAR_DOWN;
    }
    if (readings[index].currentValue == averageHistory) {
      readings[index].changeChar = CHAR_SAME;
    }

    if (readings[index].readingIndex == STORED_READING) {
      readings[index].readingIndex--;
      readings[index].enoughData = true;  // Set flag that we have all the readings
      for (int i = 0; i < STORED_READING - 1; i++) {
        readings[index].lastValue[i] = readings[index].lastValue[i + 1];  // Shift all readings down one
      }
    } else {
      readings[index].enoughData = false;
    }

    readings[index].lastValue[readings[index].readingIndex] = readings[index].currentValue;  // update with latest value
  }

  readings[index].readingIndex++;
  readings[index].lastMessageTime = millis();
  snprintf(statusMessage, CHAR_LEN, "Update received for %s", readings[index].description);

  statusMessageUpdated = true;
}

// Update humidity settings
void update_humidity(char *recMessage, int index) {

  float averageHistory;
  float totalHistory = 0.0;

  readings[index].currentValue = atof(recMessage);
  snprintf(readings[index].output, 10, "%2.0f%s", readings[index].currentValue, "%");

  if (readings[index].readingIndex == 0) {
    readings[index].changeChar = CHAR_BLANK;  // First reading of this boot
    readings[index].lastValue[0] = readings[index].currentValue;
  } else {
    for (int i = 0; i < readings[index].readingIndex; i++) {
      totalHistory = totalHistory + readings[index].lastValue[i];
    }
    averageHistory = totalHistory / readings[index].readingIndex;

    if (readings[index].currentValue > averageHistory) {
      readings[index].changeChar = CHAR_UP;
    }
    if (readings[index].currentValue < averageHistory) {
      readings[index].changeChar = CHAR_DOWN;
    }
    if (readings[index].currentValue == averageHistory) {
      readings[index].changeChar = CHAR_SAME;
    }

    if (readings[index].readingIndex == STORED_READING) {
      readings[index].readingIndex--;
      readings[index].enoughData = true;  // Set flag that we have all the readings
      for (int i = 0; i < STORED_READING - 1; i++) {
        readings[index].lastValue[i] = readings[index].lastValue[i + 1];
      }
    } else {
      readings[index].enoughData = false;
    }

    readings[index].lastValue[readings[index].readingIndex] = readings[index].currentValue;  // update with latest value
  }

  readings[index].readingIndex++;
  readings[index].lastMessageTime = millis();
  snprintf(statusMessage, CHAR_LEN, "Update received for %s", readings[index].description);
  statusMessageUpdated = true;
}

// Update battery settings
void update_battery(char *recMessage, int index) {

  float averageHistory;
  float totalHistory = 0.0;

  readings[index].currentValue = atof(recMessage);
  snprintf(readings[index].output, 10, "%2.0f%s", readings[index].currentValue, "%");

  if (readings[index].readingIndex == 0) {
    readings[index].changeChar = CHAR_BLANK;  // First reading of this boot
    readings[index].lastValue[0] = readings[index].currentValue;
    averageHistory = readings[index].currentValue;
  } else {
    for (int i = 0; i < readings[index].readingIndex; i++) {
      totalHistory = totalHistory + readings[index].lastValue[i];
    }
    averageHistory = totalHistory / readings[index].readingIndex;

    if (readings[index].readingIndex == STORED_READING) {
      readings[index].readingIndex--;
      readings[index].enoughData = true;  // Set flag that we have all the readings
      for (int i = 0; i < STORED_READING - 1; i++) {
        readings[index].lastValue[i] = readings[index].lastValue[i + 1];
      }
    } else {
      readings[index].enoughData = false;
    }

    readings[index].lastValue[readings[index].readingIndex] = readings[index].currentValue;  // update with latest value
  }

  readings[index].readingIndex++;
  readings[index].lastMessageTime = millis();
  snprintf(statusMessage, CHAR_LEN, "Update received for %s", readings[index].description);
  // Now set current value to average to reduce fluctuations
  readings[index].currentValue=averageHistory;
  statusMessageUpdated = true;
}

