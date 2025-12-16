// File: weather-esp32.ino
#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>
#include <time.h>  // Include time.h for timezone setup

#include "config.h"
#include "data.h"
#include "ota_update.h"
#include "mq135.h"
#include "calibrate.h"
#include "mqtt_handler.h"  // setupMQTT(), loopMQTT(), sendMQTT()

// --- Global Objects ---
const unsigned long SYSTEM_INFO_INTERVAL = 10000;

WiFiClient wifiClient;

String latestJson = "{}";
unsigned long lastSend = 0;

void logAppConfig() {
  addLog("--- AppConfig ---");

  addLogf("Device ID: %u", appConfig.deviceId);
  addLogf("Location: %.6f, %.6f", appConfig.latitude, appConfig.longitude);

  addLogf("WiFi SSID: %s", appConfig.wifiSSID);

  addLogf("MQTT Server: %s:%d (Enabled: %s)", appConfig.mqttServer, appConfig.mqttPort, appConfig.mqttEnabled ? "Yes" : "No");

  addLogf("Send Interval: %lu ms", appConfig.sendInterval);
  addLogf("NTP Server: %s, TimeZone: UTC%+d", appConfig.ntpServer, appConfig.timeZone);
  addLogf("Sleep Mode: %s (Duration: %lu s)", appConfig.enableSleep ? "Enabled" : "Disabled", appConfig.sleepDuration);

  addLogf("Dust Sensor Pins: LED %d, ADC %d", appConfig.dustLEDPin, appConfig.dustADCPin);
  addLogf("PMS Sensor: %s (RX %d, TX %d, SET %d)", appConfig.pmsEnabled ? "Enabled" : "Disabled", appConfig.pmsRxPin, appConfig.pmsTxPin, appConfig.pmsSetPin);
  addLogf("MQ ADC Pin: %d", appConfig.mqADCPin);

  addLogf("Auto Calibrate on Boot: %s", appConfig.autoCalibrateOnBoot ? "Yes" : "No");
  addLogf("MQ R0 (Persisted): %.4f", appConfig.mq_rzero);

  addLog("----------------");
}

// --- Deep Sleep Handler ---
void enterDeepSleep() {
  addLogf("[POWER] Entering Deep Sleep for %lu seconds...", appConfig.sleepDuration);

  // Convert seconds to microseconds
  uint64_t sleepUs = (uint64_t)appConfig.sleepDuration * 1000000;

  // Configure ESP32 to wake up after 'sleepUs' microseconds
  esp_sleep_enable_timer_wakeup(sleepUs);

  // Stop peripherals before sleeping (optional, but good practice)
  Serial.flush();

  // Enter Deep Sleep
  esp_deep_sleep_start();
}

void setup() {
  // Check if waking up from deep sleep
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
    // If waking up from Deep Sleep, quickly load config and perform data cycle
    Serial.begin(115200);
    loadConfig();
    addLogf("=== Woke up from Deep Sleep. Remaining time: %lu s ===", appConfig.sleepDuration);
    // Skip time-consuming operations like WebServer setup, OTA setup, etc.
  } else {
    // Regular boot
    Serial.begin(115200);
    addLog("=== Starting ESP32 Weather Station (First Boot) ===");

    //resetConfig();
    loadConfig();
    logAppConfig();

    // WiFi
    setupWiFi();

    // Web server & OTA
    setupWebServer();
    setupOTA();

    // BME280
    if (!bme.begin(0x76)) {
      addLog("[ERROR] BME280 not found");
      bmeInitialized = false;
    } else {
      bmeInitialized = true;
      addLog("[INFO] BME280 initialized");
    }

    // Sensors
    initDustSensor();
    // Only initialize PMS if enabled in config
    if (appConfig.pmsEnabled) initPMS7003();
    initMQ135();

    // Auto calibrate
    if (appConfig.autoCalibrateOnBoot) startCalibration();

    // Time sync (uses new ntpServer and timeZone settings)
    setupTime();

    // MQTT
    if (appConfig.mqttEnabled) setupMQTT();
    addLog("=== Setup Complete ===");
  }
}

void loop() {
  // Check if Deep Sleep is enabled. If so, only run once, then sleep.
  if (appConfig.enableSleep && esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_TIMER) {
    // If we are in sleep mode, we only run the loop once after boot to send data, then sleep.
    // We force a data send immediately (or after a short delay for sensor warm-up).
    // We handle the loop logic slightly differently to ensure exit.
  }

  maintainWiFi();

  // MQTT safe loop
  if (appConfig.mqttEnabled) loopMQTT();

  // OTA / WebServer
  ArduinoOTA.handle();
  ws.cleanupClients();
  handleOtaUrlInLoop();

  // --- Data Send Logic ---
  if (millis() - lastSend >= appConfig.sendInterval || lastSend == 0 || esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {

    // 1. Gather Data
    latestJson = getDataJson();

    // 2. Broadcast/Save Data
    notifyClients(latestJson);                        // WebSockets
    if (appConfig.mqttEnabled) sendMQTT(latestJson);  // MQTT/Queue

    lastSend = millis();

    // 3. Maintenance
    // Baseline correction (only needed if autoCalibrate is active)
    if (appConfig.autoCalibrateOnBoot) updateBaselineDriftCorrection();

    // Periodic system info
    static unsigned long lastSystemInfoSend = 0;
    if (millis() - lastSystemInfoSend >= SYSTEM_INFO_INTERVAL) {
      sendSystemInfoToClients();
      lastSystemInfoSend = millis();
    }

    // 4. Deep Sleep Check (If enabled, enter sleep after one successful data cycle)
    if (appConfig.enableSleep) {
      // Need a small delay to ensure MQTT/WS packets are sent before sleep
      delay(500);
      enterDeepSleep();
    }
  }
}

void sendSystemInfoToClients() {
  StaticJsonDocument<256> doc;

  bool connected = WiFi.isConnected();
  const char* statusMsg = connected ? "WiFi Connected" : "Connecting...";

  doc["status_msg"] = statusMsg;
  doc["uptime_seconds"] = (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) ? appConfig.sleepDuration : millis() / 1000;
  doc["wifi_rssi"] = connected ? WiFi.RSSI() : 0;
  doc["is_sleeping"] = appConfig.enableSleep;  // New flag for client display

  String jsonString;
  serializeJson(doc, jsonString);

  notifyClients(jsonString);
}
