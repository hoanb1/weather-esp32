#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <cstdarg>
#include <esp_system.h>
#include <esp_mac.h>

#include "config.h"
#include "data.h"

// --- Global Instances ---
AppConfig_t appConfig;
Preferences preferences;

LogEntry logBuffer[LOG_BUFFER_SIZE];
uint8_t logIndex = 0;

// --- Default Config ---
const AppConfig_t defaultConfig = {
  // I. GENERAL & DEVICE INFO
  .deviceId = 0,
  .latitude = 21.5,
  .longitude = 105.8,
  .stationName = "My Home Weather Station",
  .stationDescription = "ESP32 Air Quality Sensor",

  // II. NETWORK & WIFI
  .wifiSSID = "HH-1301",
  .wifiPass = "tutulala",

  // III. MQTT & DATA SERVICE
  .mqttEnabled = true,
  .mqttServer = "pi.hoan.uk",
  .mqttPort = 1883,
  .mqttPass = "74c3434e70d23c25", // Unified Token
  .queueMaxSize = 200 * 1024,  // 200 KB default
  .queueFlushInterval = 5000,  // try sending every 5s

  // IV. TIMING & POWER
  .sendInterval = 5000,
  .ntpServer = "pool.ntp.org",
  .timeZone = 7,               // UTC+7 (Vietnam time)
  .enableSleep = false,
  .sleepDuration = 300,        // 5 minutes sleep if enabled

  // V. SENSOR PINS & CONFIG
  .dustLEDPin = 15,
  .dustADCPin = 35,
  .mqADCPin = 34,

  .pmsEnabled = true,
  .pmsRxPin = 17,
  .pmsTxPin = 16,
  .pmsSetPin = -1, // No control pin by default

  // VI. SENSOR CALIBRATION & OFFSET
  .mq_rl_kohm = 1.0,
  .mq_r0_ratio_clean = 3.6,
  .mq_rzero = 10,
  .dust_baseline = 0.6,
  .dust_calibration = 1.0f,

  .autoCalibrateOnBoot = true,
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


  if (appConfig.deviceId == 0) {
    uint8_t mac_addr[6];
    esp_read_mac(mac_addr, ESP_MAC_WIFI_STA);

    appConfig.deviceId =
      ((uint32_t)mac_addr[2] << 24) | ((uint32_t)mac_addr[3] << 16) | ((uint32_t)mac_addr[4] << 8) | ((uint32_t)mac_addr[5]);

    saveConfig();
    addLogf("[CFG] Device ID was 0. Auto-re-assigned ID: %u", appConfig.deviceId);
  }
  // =================================================================

  preferences.end();
}

// --- Logging ---
void addLog(const char* msg) {
  Serial.println(msg);

  logBuffer[logIndex].message = msg;
  logIndex = (logIndex + 1) % LOG_BUFFER_SIZE;

  StaticJsonDocument<1024> doc;
  doc["type"] = "log";
  doc["msg"] = msg;

  String json;
  serializeJson(doc, json);

  // Send to WebSocket if server active
  ws.textAll(json);
}

void addLogf(const char* format, ...) {
  char buf[1024];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  addLog(buf);
}