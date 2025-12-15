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
  .wifiSSID = "HH",
  .wifiPass = "12345678",

  .mqttServer = "pi.hoan.uk",
  .mqttPort = 1883,
  .mqttUser = "sensor",
  .mqttPass = "4f9605ca60ceeff2",
  .mqttEnabled = true,

  .queueMaxSize = 200 * 1024,  // 200 KB default
  .queueFlushInterval = 5000,  // try sending every 5s

  .sendInterval = 5000,
  .ntpServer = "pool.ntp.org",

  .dustLEDPin = 15,
  .dustADCPin = 35,
  .mqADCPin = 34,

  .mq_rl_kohm = 1.0,
  .mq_r0_ratio_clean = 3.6,
  .mq_rzero = 0,
  .dust_baseline = 0,
  .dust_calibration = 1,

  .autoCalibrateOnBoot = true,


  .deviceId = 0,
  .latitude = 21.5,
  .longitude = 105.8,
  .stationName = "My Home Weather Station",
  .stationDescription = "ESP32 Air Quality Sensor",
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

    // FIX: Sử dụng esp_read_mac và chỉ lấy 4 byte cuối của MAC làm deviceId (uint32_t)
    uint8_t mac_addr[6];
    esp_read_mac(mac_addr, ESP_MAC_WIFI_STA); // Sử dụng ESP_MAC_WIFI_STA để lấy MAC của Station

    // Lấy 4 bytes cuối (4, 5, 2, 3) hoặc 4 bytes đầu (tùy ý) và chuyển thành uint32_t
    // Ở đây ta lấy 4 bytes cuối: mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]
    appConfig.deviceId =
        ((uint32_t)mac_addr[2] << 24) |
        ((uint32_t)mac_addr[3] << 16) |
        ((uint32_t)mac_addr[4] << 8) |
        ((uint32_t)mac_addr[5]);

    saveConfig();
    addLogf("[CFG] Auto-assigned Device ID: %u", appConfig.deviceId);
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