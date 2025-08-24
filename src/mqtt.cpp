#include <globals.h>
#include <klaussometer.h>
#include <cctype>
#include <cstring>

extern MqttClient mqttClient;
extern SemaphoreHandle_t mqttMutex;
extern Readings readings[];
extern int numberOfReadings;

// Get mqtt messages
void receive_mqtt_messages_t(void *pvParams) {
  int messageSize = 0;
  String topic;
  char recMessage[CHAR_LEN] = {0};
  int index;
  
  while (true) {
    // Reconnect if necessary
    if (WiFi.status() != WL_CONNECTED) {
      setup_wifi();
    }
    if (!mqttClient.connected()) {
      mqtt_connect();
    }
    // --- Protect the MQTT access with a mutex ---
    if (xSemaphoreTake(mqttMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      // We have successfully acquired the lock
      // Check for new messages only after ensuring connection is good
      messageSize = mqttClient.parseMessage();
      if (messageSize) {
        topic = mqttClient.messageTopic();
        mqttClient.read((unsigned char *)recMessage,
                        (size_t)sizeof(recMessage));
        // Give the mutex back
        xSemaphoreGive(mqttMutex); // give before calling a function that tries to log
        recMessage[messageSize] = 0;

        for (int i = 0; i < numberOfReadings; i++) {
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
    xSemaphoreGive(mqttMutex);
    // Give other tasks a chance to run
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// Update temperature settings
void update_temperature(char *recMessage, int index) {

  float averageHistory;
  float totalHistory = 0.0;
   char lowercaseDescription[CHAR_LEN];

  readings[index].currentValue = atof(recMessage);
  snprintf(readings[index].output, 10, "%2.1f", readings[index].currentValue);

  if (readings[index].readingIndex == 0) {
    readings[index].changeChar = CHAR_BLANK; // First reading of this boot
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
      readings[index].enoughData =
          true; // Set flag that we have all the readings
      for (int i = 0; i < STORED_READING - 1; i++) {
        readings[index].lastValue[i] =
            readings[index].lastValue[i + 1]; // Shift all readings down one
      }
    } else {
      readings[index].enoughData = false;
    }

    readings[index].lastValue[readings[index].readingIndex] =
        readings[index].currentValue; // update with latest value
  }

  readings[index].readingIndex++;
  readings[index].lastMessageTime = millis();

  toLowercase(readings[index].description, lowercaseDescription, CHAR_LEN);
  logAndPublish("Update received for %s temperature", lowercaseDescription);

}

// Update humidity settings
void update_humidity(char *recMessage, int index) {

  float averageHistory;
  float totalHistory = 0.0;
   char lowercaseDescription[CHAR_LEN];

  readings[index].currentValue = atof(recMessage);
  snprintf(readings[index].output, 10, "%2.0f%s", readings[index].currentValue,
           "%");

  if (readings[index].readingIndex == 0) {
    readings[index].changeChar = CHAR_BLANK; // First reading of this boot
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
      readings[index].enoughData =
          true; // Set flag that we have all the readings
      for (int i = 0; i < STORED_READING - 1; i++) {
        readings[index].lastValue[i] = readings[index].lastValue[i + 1];
      }
    } else {
      readings[index].enoughData = false;
    }

    readings[index].lastValue[readings[index].readingIndex] =
        readings[index].currentValue; // update with latest value
  }

  readings[index].readingIndex++;
  readings[index].lastMessageTime = millis();
  toLowercase(readings[index].description, lowercaseDescription, CHAR_LEN);
  logAndPublish("Update received for %s humidity", lowercaseDescription);
}

// Update battery settings
void update_battery(char *recMessage, int index) {

  float averageHistory;
  float totalHistory = 0.0;
   char lowercaseDescription[CHAR_LEN];

  readings[index].currentValue = atof(recMessage);
  snprintf(readings[index].output, 10, "%2.0f%s", readings[index].currentValue,
           "%");

  if (readings[index].readingIndex == 0) {
    readings[index].changeChar = CHAR_BLANK; // First reading of this boot
    readings[index].lastValue[0] = readings[index].currentValue;
    averageHistory = readings[index].currentValue;
  } else {
    for (int i = 0; i < readings[index].readingIndex; i++) {
      totalHistory = totalHistory + readings[index].lastValue[i];
    }
    averageHistory = totalHistory / readings[index].readingIndex;

    if (readings[index].readingIndex == STORED_READING) {
      readings[index].readingIndex--;
      readings[index].enoughData =
          true; // Set flag that we have all the readings
      for (int i = 0; i < STORED_READING - 1; i++) {
        readings[index].lastValue[i] = readings[index].lastValue[i + 1];
      }
    } else {
      readings[index].enoughData = false;
    }

    readings[index].lastValue[readings[index].readingIndex] =
        readings[index].currentValue; // update with latest value
  }

  readings[index].readingIndex++;
  toLowercase(readings[index].description, lowercaseDescription, CHAR_LEN);
  logAndPublish("Update received for %s battery", lowercaseDescription);
  // Now set current value to average to reduce fluctuations
  readings[index].currentValue = averageHistory;
}

char* toLowercase(const char* source, char* buffer, size_t bufferSize) {
    if (source == nullptr || buffer == nullptr || bufferSize == 0) {
        return nullptr;
    }
    strncpy(buffer, source, bufferSize - 1);
    buffer[bufferSize - 1] = '\0';
    for (size_t i = 0; i < strlen(buffer); ++i) {
        buffer[i] = tolower(buffer[i]);
    }
    return buffer;
}
