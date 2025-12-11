// File: weather-esp32.ino
#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>

#include "config.h"
#include "data.h"
#include "ota_update.h"
#include "mq135.h"
#include "calibrate.h"
#include "mqtt_handler.h"  // loopMQTT(), sendMQTT()

// --- Global Objects ---
const unsigned long SYSTEM_INFO_INTERVAL = 10000;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

String latestJson = "{}";
unsigned long lastSend = 0;

void logAppConfig() {
  addLog("--- AppConfig ---");

  addLogf("WiFi SSID: %s", appConfig.wifiSSID);
  addLogf("MQTT Server: %s:%d", appConfig.mqttServer, appConfig.mqttPort);
  addLogf("MQTT Topic: %s", appConfig.mqttTopic);
  addLogf("Send Interval: %lu ms", appConfig.sendInterval);

  addLogf("Dust LED Pin: %d, Dust ADC Pin: %d", appConfig.dustLEDPin, appConfig.dustADCPin);
  addLogf("MQ135 ADC Pin: %d", appConfig.mqADCPin);

  addLogf("Device ID: %s", appConfig.deviceId);
  addLogf("Location: %.6f, %.6f", appConfig.latitude, appConfig.longitude);

  addLog("----------------");
}

void setup() {
  Serial.begin(115200);
  addLog("=== Starting ESP32 Weather Station ===");

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
  initMQ135();

  // Auto calibrate
  if (appConfig.autoCalibrateOnBoot) startCalibration();

  // Time sync
  setupTime();

  // MQTT
  if (appConfig.mqttEnabled) setupMQTT();  // only set server if enabled
  addLog("=== Setup Complete ===");
}

void loop() {
  maintainWiFi();

  // MQTT safe loop
  loopMQTT();

  // OTA
  ArduinoOTA.handle();
  ws.cleanupClients();

  // Send sensor data
  if (millis() - lastSend >= appConfig.sendInterval) {
    latestJson = getDataJson();
    notifyClients(latestJson);

    if (appConfig.mqttEnabled) sendMQTT(latestJson);

    lastSend = millis();

    // Baseline correction
    if (appConfig.autoCalibrateOnBoot) updateBaselineDriftCorrection();

    // Periodic system info
    static unsigned long lastSystemInfoSend = 0;
    if (millis() - lastSystemInfoSend >= SYSTEM_INFO_INTERVAL) {
      sendSystemInfoToClients();
      lastSystemInfoSend = millis();
    }
  }
}

void sendSystemInfoToClients() {
  StaticJsonDocument<256> doc;

  bool connected = WiFi.isConnected();
  const char* statusMsg = connected ? "WiFi Connected" : "Connecting...";

  doc["status_msg"] = statusMsg;
  doc["uptime_seconds"] = millis() / 1000;
  doc["wifi_rssi"] = connected ? WiFi.RSSI() : 0;

  String jsonString;
  serializeJson(doc, jsonString);

  notifyClients(jsonString);
}
