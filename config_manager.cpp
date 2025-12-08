// config_manager.cpp

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <cstdarg>

#include "data.h"
#include "config.h"

AppConfig_t appConfig;
Preferences preferences;

LogEntry logBuffer[LOG_BUFFER_SIZE];
uint8_t logIndex = 0;

// Default Configuration
const AppConfig_t defaultConfig = {
    .wifiSSID = "HH",
    .wifiPass = "12345678",
    .mqttServer = "pi.hoan.uk",
    .mqttPort = 1883,
    .mqttUser = "weather",
    .mqttPass = "hoan1234",
    .mqttTopic = "weather/data",
    .sendInterval = 5000,
    .ntpServer = "pool.ntp.org",

    .dustLEDPin = 15,
    .dustADCPin = 35,
    .mqADCPin = 34,

    .mq_rl_kohm = 10.0,
    .mq_r0_ratio_clean = 3.6,
    .tvoc_a_curve = 116.602,
    .tvoc_b_curve = -2.769,
    .mq_rzero = 0,

    .deviceId = "01",
    .latitude = 21.5,
    .longitude = 105.8,
};

void resetConfig() {
    addLog("[CFG] Resetting to default config...");
    memcpy(&appConfig, &defaultConfig, sizeof(AppConfig_t));
    saveConfig();
}

void saveConfig() {
    preferences.begin(PREFERENCES_NAMESPACE, false);
    preferences.putBytes("config", &appConfig, sizeof(AppConfig_t));
    preferences.end();

    addLog("[CFG] Configuration saved to NVS");
}

void loadConfig() {
    preferences.begin(PREFERENCES_NAMESPACE, true);
    size_t savedSize = preferences.getBytesLength("config");

    if (savedSize == sizeof(AppConfig_t)) {
        preferences.getBytes("config", &appConfig, sizeof(AppConfig_t));
        addLog("[CFG] Configuration loaded from NVS");
    } else {
        addLog("[CFG] No valid config found in NVS — loading defaults");
        resetConfig();
    }

    preferences.end();
}

void addLog(const char* msg) {
    Serial.println(msg);

    logBuffer[logIndex].message = msg;
    logIndex = (logIndex + 1) % LOG_BUFFER_SIZE;

    StaticJsonDocument<256> doc;
    doc["type"] = "log";
    doc["msg"] = msg;

    String json;
    serializeJson(doc, json);

    ws.textAll(json);
}

void addLogf(const char* format, ...) {
    char buf[256];

    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    addLog(buf);
}
