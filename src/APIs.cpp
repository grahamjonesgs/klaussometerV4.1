
#include "globals.h"

extern Weather weather;
extern UV uv;
extern Solar solar;
extern Preferences storage;
extern NTPClient timeClient;

HTTPClient http;
String token = "";

// The semaphore to protect the HTTPClient object
extern SemaphoreHandle_t httpMutex;

// Get UV from weatherbit.io
void get_uv_t(void* pvParameters) {

    const char apiKey[] = WEATHERBIT_API;
    while (true) {
        if (weather.isDay) {
            if (now() - uv.updateTime > UV_UPDATE_INTERVAL) {
                if (xSemaphoreTake(httpMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                    http.begin("https://api.weatherbit.io/v2.0/"
                               "current?city_id=3369157&key=" +
                               String(apiKey));
                    int httpCode = http.GET();
                    if (httpCode == HTTP_CODE_OK) {
                        String payload = http.getString();
                        JsonDocument root;
                        deserializeJson(root, payload);
                        float UV = root["data"][0]["uv"];
                        uv.index = UV;
                        uv.updateTime = now();
                        logAndPublish("UV updated");
                        timeClient.getFormattedTime().toCharArray(uv.time_string, CHAR_LEN);
                        http.end();
                        xSemaphoreGive(httpMutex);
                    } else {
                        http.end();
                        xSemaphoreGive(httpMutex);
                        errorPublish("[HTTP] GET UV failed, error: %s\n", http.errorToString(httpCode).c_str());
                        logAndPublish("UV updated failed");
                        vTaskDelay(pdMS_TO_TICKS(30000)); // Stop calling too often for errors
                    }
                }
            }
        } else {
            uv.index = 0.0;
            if (weather.updateTime > 0) { // Only update if the weather is valid so
                // the day / night is valid
                uv.updateTime = now();
            }
            timeClient.getFormattedTime().toCharArray(uv.time_string, CHAR_LEN);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void get_weather_t(void* pvParameters) {
    while (true) {
        if (now() - weather.updateTime > WEATHER_UPDATE_INTERVAL) {
            if (xSemaphoreTake(httpMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                http.begin("https://api.open-meteo.com/v1/"
                           "forecast?latitude=-33.9258&longitude=18.4232&daily="
                           "temperature_2m_"
                           "max,temperature_2m_min,sunrise,sunset,uv_index_max&models="
                           "ukmo_uk_"
                           "deterministic_2km,ncep_gfs013&current=temperature_2m,is_day,"
                           "weather_code,wind_speed_10m,wind_direction_10m&timezone=auto&"
                           "forecast_days=1");
                int httpCode = http.GET();
                if (httpCode == HTTP_CODE_OK) {
                    String payload = http.getString();
                    JsonDocument root;
                    deserializeJson(root, payload);

                    float weatherTemperature = root["current"]["temperature_2m"];
                    float weatherWindDir = root["current"]["wind_direction_10m"];
                    float weatherWindSpeed = root["current"]["wind_speed_10m"];
                    float weatherMaxTemp = root["daily"]["temperature_2m_max"][0];
                    float weatherMinTemp = root["daily"]["temperature_2m_min"][0];
                    bool weatherIsDay = root["current"]["is_day"];
                    int weatherCode = root["current"]["weather_code"];

                    weather.temperature = weatherTemperature;
                    weather.windSpeed = weatherWindSpeed;
                    weather.maxTemp = weatherMaxTemp;
                    weather.minTemp = weatherMinTemp;
                    weather.isDay = weatherIsDay;
                    strncpy(weather.description, wmoToText(weatherCode, weatherIsDay), CHAR_LEN);
                    strncpy(weather.windDir, degreesToDirection(weatherWindDir), CHAR_LEN);

                    weather.updateTime = now();
                    timeClient.getFormattedTime().toCharArray(weather.time_string, CHAR_LEN);
                    http.end();
                    xSemaphoreGive(httpMutex);
                    logAndPublish("Weather updated");
                } else {
                    http.end();
                    xSemaphoreGive(httpMutex);
                    errorPublish("[HTTP] GET current weather failed, error: %s\n", http.errorToString(httpCode).c_str());
                    logAndPublish("Weather updated failed");
                    vTaskDelay(pdMS_TO_TICKS(30000)); // Stop calling too often for errors
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

const char* degreesToDirection(double degrees) {
    // Normalize the degrees to a 0-360 range
    // The fmod function handles floating point remainder.
    degrees = fmod(degrees, 360.0);
    if (degrees < 0) {
        degrees += 360.0;
    }

    // Determine the direction based on 45-degree sectors.
    // We add 22.5 to shift the starting point so that North is centered on 0.
    // This simplifies the logic by making the ranges positive.
    double shiftedDegrees = degrees + 22.5;

    if (shiftedDegrees >= 360) {
        shiftedDegrees -= 360;
    }

    if (shiftedDegrees < 45) {
        return "N";
    } else if (shiftedDegrees < 90) {
        return "NE";
    } else if (shiftedDegrees < 135) {
        return "E";
    } else if (shiftedDegrees < 180) {
        return "SE";
    } else if (shiftedDegrees < 225) {
        return "S";
    } else if (shiftedDegrees < 270) {
        return "SW";
    } else if (shiftedDegrees < 315) {
        return "W";
    } else {
        return "NW";
    }
}

const char* wmoToText(int code, bool isDay) {
    switch (code) {
    case 0:
        return isDay ? "Sunny" : "Clear";
    case 1:
        return isDay ? "Mainly sunny" : "Mostly clear";
    case 2:
        return isDay ? "Partly cloudy" : "Partly cloudy";
    case 3:
        return "Overcast";
    case 45:
        return "Fog";
    case 48:
        return "Depositing rime fog";
    case 51:
        return "Light drizzle";
    case 53:
        return "Moderate drizzle";
    case 55:
        return "Dense drizzle";
    case 56:
        return "Light freezing drizzle";
    case 57:
        return "Dense freezing drizzle";
    case 61:
        return "Slight rain";
    case 63:
        return "Moderate rain";
    case 65:
        return "Heavy rain";
    case 66:
        return "Light freezing rain";
    case 67:
        return "Heavy freezing rain";
    case 71:
        return "Slight snow fall";
    case 73:
        return "Moderate snow fall";
    case 75:
        return "Heavy snow fall";
    case 77:
        return "Snow grains";
    case 80:
        return "Slight rain showers";
    case 81:
        return "Moderate rain showers";
    case 82:
        return "Violent rain showers";
    case 85:
        return "Slight snow showers";
    case 86:
        return "Heavy snow showers";
    case 95:
        return "Thunderstorm";
    case 96:
        return "Thunderstorm with slight hail";
    case 99:
        return "Thunderstorm with heavy hail";
    default:
        return "Unknown weather code or not provided in the Gist.";
    }
}

// Get current solar values from Solarman
void get_current_solar_t(void* pvParameters) {

    String solar_url = SOLAR_URL;
    String solar_appid = SOLAR_APPID;
    String solar_secret = SOLAR_SECRET;
    String solar_username = SOLAR_USERNAME;
    String solar_passhash = SOLAR_PASSHASH;
    String solar_stationid = SOLAR_STATIONID;

    // Get station status
    while (true) {
        if (now() - solar.currentUpdateTime > SOLAR_CURRENT_UPDATE_INTERVAL) {
            if (xSemaphoreTake(httpMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                http.begin("https://" + solar_url + "/station/v1.0/realTime?language=en");
                http.addHeader("Content-Type", "application/json");
                http.addHeader("Authorization", token);
                int httpCode = http.POST("{\n\"stationId\" : \"" + solar_stationid + "\"\n}");
                vTaskDelay(pdMS_TO_TICKS(100));
                if (httpCode == HTTP_CODE_OK) {
                    String payload = http.getString();
                    JsonDocument root;
                    deserializeJson(root, payload);
                    bool rec_success = root["success"];
                    if (rec_success == true) {
                        float rec_batteryCharge = root["batterySoc"];
                        float rec_usingPower = root["usePower"];
                        float rec_gridPower = root["wirePower"];
                        float rec_batteryPower = root["batteryPower"];
                        time_t rec_time = root["lastUpdateTime"];
                        float rec_solarPower = root["generationPower"];

                        struct tm ts;
                        char time_buf[CHAR_LEN];

                        rec_time += TIME_OFFSET;
                        localtime_r(&rec_time, &ts);
                        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &ts);
                        solar.currentUpdateTime = now();
                        solar.solarPower = rec_solarPower / 1000;
                        solar.batteryPower = rec_batteryPower / 1000;
                        solar.usingPower = rec_usingPower / 1000;
                        solar.batteryCharge = rec_batteryCharge;
                        solar.gridPower = rec_gridPower / 1000;
                        strncpy(solar.time, time_buf, CHAR_LEN);

                        // Reset at midnight
                        if (timeClient.getHours() == 0 && solar.minmax_reset == false) {
                            solar.today_battery_min = 100;
                            solar.today_battery_max = 0;
                            solar.minmax_reset = true;
                            storage.begin("KO");
                            storage.remove("solarmin");
                            storage.remove("solarmmax");
                            storage.putFloat("solarmin", solar.today_battery_min);
                            storage.putFloat("solarmax", solar.today_battery_max);
                            storage.end();
                        } else {
                            if (timeClient.getHours() != 0) {
                                solar.minmax_reset = false;
                            }
                        }
                        // Set minimum
                        if ((solar.batteryCharge < solar.today_battery_min) && solar.batteryCharge != 0) {
                            solar.today_battery_min = solar.batteryCharge;
                            storage.begin("KO");
                            storage.remove("solarmin");
                            storage.putFloat("solarmin", solar.today_battery_min);
                            storage.end();
                        }
                        // Set maximum
                        if (solar.batteryCharge > solar.today_battery_max) {
                            solar.today_battery_max = solar.batteryCharge;
                            storage.begin("KO");
                            storage.remove("solarmax");
                            storage.putFloat("solarmax", solar.today_battery_max);
                            storage.end();
                        }
                        http.end();
                        xSemaphoreGive(httpMutex);
                        logAndPublish("Solar status updated");
                    } else {
                        if (root["msg"].is<const char*>()) {
                            const char* msg = root["msg"];
                            if (*msg != 0) {
                                if (strcmp(msg, "auth invalid token")) {
                                    vTaskDelay(pdMS_TO_TICKS(100));
                                    token = "";
                                    while (token.length() == 0) {
                                        http.begin("https://" + solar_url + "/account/v1.0/token?" + "appId=" + solar_appid);
                                        http.addHeader("Content-Type", "application/json");

                                        int httpCode_token = http.POST("{\n\"appSecret\" : \"" + solar_secret + "\", \n\"email\" : \"" + solar_username + "\",\n\"password\" : \"" +
                                                                 solar_passhash + "\"\n}");
                                        vTaskDelay(pdMS_TO_TICKS(100));
                                        if (httpCode_token == HTTP_CODE_OK) {
                                            String payload = http.getString();
                                            JsonDocument root;
                                            deserializeJson(root, payload);
                                            if (root["access_token"].is<const char*>()) {
                                                const char* rec_token = root["access_token"];
                                                logAndPublish("Solar token "
                                                              "obtained");
                                                token = rec_token;
                                                token = "bearer " + token;
                                                http.end();
                                                xSemaphoreGive(httpMutex);
                                            } else {
                                                http.end();
                                                xSemaphoreGive(httpMutex);
                                                logAndPublish("Solar token error");
                                                vTaskDelay(pdMS_TO_TICKS(30000)); // Stop asking
                                                                                  // too often
                                                                                  // for token
                                            }
                                        } else {
                                            http.end();
                                            xSemaphoreGive(httpMutex);
                                            errorPublish("[HTTP] GET solar token "
                                                         "failed, error: %s\n",
                                                         http.errorToString(httpCode).c_str());
                                            logAndPublish("Getting solar token failed");
                                            vTaskDelay(pdMS_TO_TICKS(30000)); // Stop calling too
                                                                              // often
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    http.end();
                    xSemaphoreGive(httpMutex);
                    errorPublish("[HTTP] GET solar status failed, error: %s\n", http.errorToString(httpCode).c_str());
                    String payload = http.getString();
                    logAndPublish("Getting solar status failed");
                    vTaskDelay(pdMS_TO_TICKS(30000)); // Stop calling too often for errors
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Get current solar values from Solarman
void get_daily_solar_t(void* pvParameters) {

    String solar_url = SOLAR_URL;
    String solar_username = SOLAR_USERNAME;
    String solar_stationid = SOLAR_STATIONID;
    char currentDate[CHAR_LEN];
    char currentYearMonth[CHAR_LEN];
    char previousMonthYearMonth[CHAR_LEN];

    // Get station status
    while (true) {
        if ((now() - solar.dailyUpdateTime > SOLAR_DAILY_UPDATE_INTERVAL) && (token != "")) {
            if (xSemaphoreTake(httpMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                /*
                  timeType 1 with start and end date of today gives array of size
                  "total", then in stationDataItems -> batterySoc to get battery
                  min/max for today timeType 2 with start and end date of today
                  gives today's buy amount as stationDataItems -> buyValue
                  timeType 3 with start and end date of today (but now date only
                  year month) gives this months's buy amount as stationDataItems
                  -> buyValue
                  */
                time_t now_time = timeClient.getEpochTime();
                struct tm CurrenTimeInfo;
                localtime_r(&now_time, &CurrenTimeInfo);
                time_t previousMonth = now_time - 30 * 24 * 3600; // One month ago
                struct tm previousMonthTimeInfo;
                localtime_r(&previousMonth, &previousMonthTimeInfo);

                strftime(currentDate, sizeof(currentDate), "%Y-%m-%d", &CurrenTimeInfo);
                strftime(currentYearMonth, sizeof(currentYearMonth), "%Y-%m", &CurrenTimeInfo);
                strftime(previousMonthYearMonth, sizeof(previousMonthYearMonth), "%Y-%m", &previousMonthTimeInfo);

                // Get the today buy amount (timetype 2)
                http.begin("https://" + solar_url + "/station/v1.0/history?language=en");
                http.addHeader("Content-Type", "application/json");
                http.addHeader("Authorization", token);

                int httpCode = http.POST("{\n\"stationId\" : \"" + solar_stationid + "\",\n\"timeType\" : 2,\n\"startTime\" : \"" + currentDate + "\",\n\"endTime\" : \"" +
                                         currentDate + "\"\n}");
                vTaskDelay(pdMS_TO_TICKS(100));
                if (httpCode == HTTP_CODE_OK) {
                    String payload = http.getString();
                    JsonDocument root;
                    deserializeJson(root, payload);
                    bool rec_success = root["success"];
                    if (rec_success == true) {
                        float today_buy = root["stationDataItems"][0]["buyValue"];
                        solar.today_buy = today_buy;
                        logAndPublish("Solar today's buy value updated");
                        solar.dailyUpdateTime = now();
                    } else {
                        String rec_msg = root["msg"];
                    }
                } else {
                    errorPublish("[HTTP] GET solar today buy value failed, error: %s\n", http.errorToString(httpCode).c_str());
                    String payload = http.getString();
                    logAndPublish("Getting solar today buy value failed");
                    vTaskDelay(pdMS_TO_TICKS(30000)); // Stop calling too often for errors
                }
                http.end();
                xSemaphoreGive(httpMutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Get current solar values from Solarman
void get_monthly_solar_t(void* pvParameters) {

    String solar_url = SOLAR_URL;
    String solar_username = SOLAR_USERNAME;
    String solar_stationid = SOLAR_STATIONID;
    char currentDate[CHAR_LEN];
    char currentYearMonth[CHAR_LEN];
    char previousMonthYearMonth[CHAR_LEN];

    // Get station status
    while (true) {
        if ((now() - solar.monthlyUpdateTime > SOLAR_MONTHLY_UPDATE_INTERVAL) && (token != "")) {
            if (xSemaphoreTake(httpMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                /*
                  timeType 1 with start and end date of today gives array of size
                  "total", then in stationDataItems -> batterySoc to get battery
                  min/max for today timeType 2 with start and end date of today
                  gives today's buy amount as stationDataItems -> buyValue
                  timeType 3 with start and end date of today (but now date only
                  year month) gives this months's buy amount as stationDataItems
                  -> buyValue
                  */
                time_t now_time = timeClient.getEpochTime();
                struct tm CurrentTimeInfo;
                localtime_r(&now_time, &CurrentTimeInfo);
                time_t previousMonth = now_time - 30 * 24 * 3600; // One month ago
                struct tm previousMonthTimeInfo;
                localtime_r(&previousMonth, &previousMonthTimeInfo);

                strftime(currentDate, sizeof(currentDate), "%Y-%m-%d", &CurrentTimeInfo);
                strftime(currentYearMonth, sizeof(currentYearMonth), "%Y-%m", &CurrentTimeInfo);
                strftime(previousMonthYearMonth, sizeof(previousMonthYearMonth), "%Y-%m", &previousMonthTimeInfo);

                // Get month buy value timeType 3
                http.begin("https://" + solar_url + "/station/v1.0/history?language=en");
                http.addHeader("Content-Type", "application/json");
                http.addHeader("Authorization", token);

                int httpCode = http.POST("{\n\"stationId\" : \"" + solar_stationid + "\",\n\"timeType\" : 3,\n\"startTime\" : \"" + currentYearMonth + "\",\n\"endTime\" : \"" +
                                         currentYearMonth + "\"\n}");
                vTaskDelay(pdMS_TO_TICKS(100));
                if (httpCode == HTTP_CODE_OK) {
                    String payload = http.getString();
                    JsonDocument root;
                    deserializeJson(root, payload);
                    bool rec_success = root["success"];
                    if (rec_success == true) {
                        float month_buy = root["stationDataItems"][0]["buyValue"];

                        solar.month_buy = month_buy;
                        solar.monthlyUpdateTime = now();
                        logAndPublish("Solar month's buy value updated");
                    }
                } else {
                    errorPublish("[HTTP] GET solar month buy value failed, error: %s\n", http.errorToString(httpCode).c_str());
                    String payload = http.getString();
                    logAndPublish("Getting solar month buy value failed");
                    vTaskDelay(pdMS_TO_TICKS(30000)); // Stop calling too often for errors
                }
                http.end();
                xSemaphoreGive(httpMutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}