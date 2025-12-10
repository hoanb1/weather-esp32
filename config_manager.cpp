#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <cstdarg>

#include "config.h"
#include "data.h"

// --- Global Instances ---
AppConfig_t appConfig;
Preferences preferences;

LogEntry logBuffer[LOG_BUFFER_SIZE];
uint8_t logIndex = 0;

// --- Default Config ---
const AppConfig_t defaultConfig = {
  .wifiSSID = "",
  .wifiPass = "",

  .mqttServer = "pi.hoan.uk",
  .mqttPort = 1883,
  .mqttUser = "sensor",
  .mqttPass = "pass1234",
  .mqttTopic = "weather/data",
  .mqttEnabled = true,

  .sendInterval = 5000,
  .ntpServer = "pool.ntp.org",

  .dustLEDPin = 15,
  .dustADCPin = 35,
  .mqADCPin = 34,

  .mq_rl_kohm = 10.0,
  .mq_r0_ratio_clean = 3.6,
  .mq_rzero = 0,
  .dust_baseline = 0,

  .autoCalibrateOnBoot = true,


  .deviceId = "01",
  .latitude = 21.5,
  .longitude = 105.8,
};

// --- Reset config to defaults ---
void resetConfig() {
  addLog("[CFG] Resetting to default configuration...");
  memcpy(&appConfig, &defaultConfig, sizeof(AppConfig_t));
  saveConfig();
}

// --- Save config to NVS ---
void saveConfig() {
  preferences.begin(PREFERENCES_NAMESPACE, false);
  preferences.putBytes("config", &appConfig, sizeof(AppConfig_t));
  preferences.end();
  addLog("[CFG] Configuration saved to NVS");
}

// --- Load config from NVS ---
void loadConfig() {
  preferences.begin(PREFERENCES_NAMESPACE, true);

  size_t savedSize = preferences.getBytesLength("config");

  if (savedSize == sizeof(AppConfig_t)) {
    preferences.getBytes("config", &appConfig, sizeof(AppConfig_t));
    addLog("[CFG] Config loaded from NVS");
  } else {
    addLog("[CFG] No valid config — using defaults");
    memcpy(&appConfig, &defaultConfig, sizeof(AppConfig_t));
  }

  preferences.end();
}

// --- Logging ---
void addLog(const char* msg) {
  Serial.println(msg);

  logBuffer[logIndex].message = msg;
  logIndex = (logIndex + 1) % LOG_BUFFER_SIZE;

  StaticJsonDocument<256> doc;
  doc["type"] = "log";
  doc["msg"] = msg;

  String json;
  serializeJson(doc, json);

  // Send to WebSocket if server active
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