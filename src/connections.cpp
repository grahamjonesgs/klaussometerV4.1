#include <globals.h>
#include <klaussometer.h>

extern WiFiClient espClient;
extern MqttClient mqttClient;
extern WiFiUDP ntpUDP;
extern NTPClient timeClient;
extern Readings readings[];
extern char statusMessage[];
extern bool statusMessageUpdated;
extern int numberOfReadings;

void setup_wifi() {
  int counter = 1;
  //WiFi.useStaticBuffers(true);
  WiFi.mode(WIFI_STA);
  //WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("scan start");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
      Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
      delay(10);
    }
  }
  Serial.println("");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.persistent(false);
//WiFi.setAutoConnect(false);
WiFi.setAutoReconnect(true);
WiFi.setTxPower(WIFI_POWER_2dBm);


  while (WiFi.status() != WL_CONNECTED) {
    if (lv_is_initialized()) {
      lv_obj_set_style_text_color(ui_WiFiStatus, lv_color_hex(COLOR_RED), LV_PART_MAIN);
    }
    counter++;
    Serial.println("Trying to connect to \n" + String(WIFI_SSID));
    WiFi.disconnect();
    WiFi.reconnect();
    
    if (counter > WIFI_RETRIES) {
      Serial.println("Restarting due to WiFi connection errors");
      ESP.restart();
    }
    delay(3000);
  }
  if (lv_is_initialized()) {
    lv_obj_set_style_text_color(ui_WiFiStatus, lv_color_hex(COLOR_GREEN), LV_PART_MAIN);
  }
  Serial.println("WiFi is OK => ESP32 IP address is: " + WiFi.localIP().toString());
}

void mqtt_connect() {

  mqttClient.setUsernamePassword(MQTT_USER, MQTT_PASSWORD);
  Serial.println();
  Serial.print("Attempting to connect to the MQTT broker : ");
  Serial.println(MQTT_SERVER);
  if (!mqttClient.connected()) {
    if (!mqttClient.connect(MQTT_SERVER, 1883)) {
      Serial.print("MQTT connection failed");
      if (lv_is_initialized()) {
        lv_obj_set_style_text_color(ui_ServerStatus, lv_color_hex(COLOR_RED), LV_PART_MAIN);
      }
      delay(2000);
      return;
    }
  }

  Serial.println("Connected to the MQTT broker");
  if (lv_is_initialized()) {
    lv_obj_set_style_text_color(ui_ServerStatus, lv_color_hex(COLOR_GREEN), LV_PART_MAIN);
  }

  for (int i = 0; i < numberOfReadings; i++) {
    mqttClient.subscribe(readings[i].topic);
    readings[i].lastMessageTime = millis();
  }
}

void time_init() {
  timeClient.begin();
  for (int i = 0; i < TIME_RETRIES; i++) {
    bool retcode;
    retcode = timeClient.forceUpdate();
    if (retcode == true) {
      break;
    } else {
      Serial.printf("Error getting time, code is %i\n", retcode);
    }
    timeClient.begin();
  }
  setTime(timeClient.getEpochTime());

  Serial.print(F("Time is : "));
  Serial.println(timeClient.getFormattedTime());
}