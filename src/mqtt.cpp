#include <globals.h>
#include <klaussometer.h>

#include <cctype>
#include <cstring>

extern MqttClient mqttClient;
extern SemaphoreHandle_t mqttMutex;
extern Readings readings[];
extern int numberOfReadings;

// Get mqtt messages
void receive_mqtt_messages_t(void* pvParams) {
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
        if (xSemaphoreTake(mqttMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
            messageSize = mqttClient.parseMessage();
            if (messageSize) {
                topic = mqttClient.messageTopic();
                mqttClient.read((unsigned char*)recMessage, sizeof(recMessage));
                // Give the mutex back
                xSemaphoreGive(mqttMutex); // give before calling a function that tries to log
                recMessage[messageSize] = 0;

                /*for (int i = 0; i < numberOfReadings; i++) {
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
                }*/
                for (int i = 0; i < numberOfReadings; i++) {
                    if (topic == String(readings[i].topic)) {
                        index = i;
                        if (readings[i].dataType == DATA_TEMPERATURE || readings[i].dataType == DATA_HUMIDITY || readings[i].dataType == DATA_BATTERY) {
                            update_readings(recMessage, index, readings[i].dataType);
                        }
                    }
                }
            } else {
                // No message
                xSemaphoreGive(mqttMutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Update temperature settings
void update_temperature(char* recMessage, int index) {
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
            readings[index].enoughData = true; // Set flag that we have all the readings
            for (int i = 0; i < STORED_READING - 1; i++) {
                readings[index].lastValue[i] = readings[index].lastValue[i + 1]; // Shift all readings down one
            }
        } else {
            readings[index].enoughData = false;
        }

        readings[index].lastValue[readings[index].readingIndex] = readings[index].currentValue; // update with latest value
    }

    readings[index].readingIndex++;
    readings[index].lastMessageTime = millis();

    toLowercase(readings[index].description, lowercaseDescription, CHAR_LEN);
    logAndPublish("Update received for %s temperature", lowercaseDescription);
}

// Update humidity settings
void update_humidity(char* recMessage, int index) {
    float averageHistory;
    float totalHistory = 0.0;
    char lowercaseDescription[CHAR_LEN];

    readings[index].currentValue = atof(recMessage);
    snprintf(readings[index].output, 10, "%2.0f%s", readings[index].currentValue, "%");

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
            readings[index].enoughData = true; // Set flag that we have all the readings
            for (int i = 0; i < STORED_READING - 1; i++) {
                readings[index].lastValue[i] = readings[index].lastValue[i + 1];
            }
        } else {
            readings[index].enoughData = false;
        }

        readings[index].lastValue[readings[index].readingIndex] = readings[index].currentValue; // update with latest value
    }

    readings[index].readingIndex++;
    readings[index].lastMessageTime = millis();
    toLowercase(readings[index].description, lowercaseDescription, CHAR_LEN);
    logAndPublish("Update received for %s humidity", lowercaseDescription);
}

// Update battery settings
void update_battery(char* recMessage, int index) {
    float averageHistory;
    float totalHistory = 0.0;
    char lowercaseDescription[CHAR_LEN];

    readings[index].currentValue = atof(recMessage);
    snprintf(readings[index].output, 10, "%2.0f%s", readings[index].currentValue, "%");

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
            readings[index].enoughData = true; // Set flag that we have all the readings
            for (int i = 0; i < STORED_READING - 1; i++) {
                readings[index].lastValue[i] = readings[index].lastValue[i + 1];
            }
        } else {
            readings[index].enoughData = false;
        }

        readings[index].lastValue[readings[index].readingIndex] = readings[index].currentValue; // update with latest value
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
void update_readings(char* recMessage, int index, int dataType) {
    float averageHistory;
    float totalHistory = 0.0;
    char lowercaseDescription[CHAR_LEN];
    const char* log_message_suffix;
    const char* format_string;

    readings[index].currentValue = atof(recMessage);

    // Set format string and log suffix based on data type
    switch (dataType) {
    case DATA_TEMPERATURE:
        format_string = "%2.1f";
        log_message_suffix = "temperature";
        break;
    case DATA_HUMIDITY:
    case DATA_BATTERY:
        format_string = "%2.0f%s";
        log_message_suffix = (dataType == DATA_HUMIDITY) ? "humidity" : "battery";
        break;
    default:
        // Handle unknown data type
        return;
    }

    if (dataType == DATA_HUMIDITY || dataType == DATA_BATTERY) {
        snprintf(readings[index].output, 10, format_string, readings[index].currentValue, "%");
    } else {
        snprintf(readings[index].output, 10, format_string, readings[index].currentValue);
    }

    if (readings[index].readingIndex == 0) {
        readings[index].changeChar = CHAR_BLANK;
        readings[index].lastValue[0] = readings[index].currentValue;
        if (dataType == DATA_BATTERY) {
            averageHistory = readings[index].currentValue;
        }
    } else {
        for (int i = 0; i < readings[index].readingIndex; i++) {
            totalHistory += readings[index].lastValue[i];
        }
        averageHistory = totalHistory / readings[index].readingIndex;

        // Only update change character for temperature and humidity
        if (dataType == DATA_TEMPERATURE || dataType == DATA_HUMIDITY) {
            if (readings[index].currentValue > averageHistory) {
                readings[index].changeChar = CHAR_UP;
            } else if (readings[index].currentValue < averageHistory) {
                readings[index].changeChar = CHAR_DOWN;
            } else {
                readings[index].changeChar = CHAR_SAME;
            }
        }
    }

    if (readings[index].readingIndex == STORED_READING) {
        readings[index].readingIndex--;
        readings[index].enoughData = true;
        for (int i = 0; i < STORED_READING - 1; i++) {
            readings[index].lastValue[i] = readings[index].lastValue[i + 1];
        }
    } else {
        readings[index].enoughData = false;
    }

    readings[index].lastValue[readings[index].readingIndex] = readings[index].currentValue;
    readings[index].readingIndex++;
    readings[index].lastMessageTime = millis();

    // Specific logic for battery
    if (dataType == DATA_BATTERY) {
        readings[index].currentValue = averageHistory;
    }

    toLowercase(readings[index].description, lowercaseDescription, CHAR_LEN);
    logAndPublish("Update received for %s %s", lowercaseDescription, log_message_suffix);
}