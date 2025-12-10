//weather-esp32.ino
#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>

#include "config.h"
#include "data.h"
#include "ota_update.h"
#include "mq135.h"

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

  // Dust sensor
  initDustSensor();
  if (dustSensor) {
    float baseline = dustSensor->getBaselineCandidate();
    dustSensor->setBaseline(baseline);
    addLogf("[BASELINE] Dust baseline set to %.4f", dustSensor->getBaseline());
  }

  // MQ135 sensor
  initMQ135();

  // BME280
  if (!bme.begin(0x76)) {
    addLog("[ERROR] BME280 not found");
    bmeInitialized = false;
  } else {
    bmeInitialized = true;
    addLog("[INFO] BME280 initialized");
  }

  // Time sync
  setupTime();

  // MQTT
  if (!appConfig.mqttEnabled) {
    addLog("[MQTT] Disabled, skipping publish");
  } else {
    mqttClient.setServer(appConfig.mqttServer, appConfig.mqttPort);
  }
  addLog("=== Setup Complete ===");
}

void loop() {
  // WiFi & MQTT
  maintainWiFi();
  
  if (appConfig.mqttEnabled) {
    reconnectMQTT();
  }
  mqttClient.loop();

  // OTA
  ArduinoOTA.handle();
  ws.cleanupClients();

  // Dust baseline adjustment
  static unsigned long lastBaselineCheck = 0;
  if (millis() - lastBaselineCheck > 60000 && dustSensor) {
    float candidate = dustSensor->getBaselineCandidate();
    if (candidate < dustSensor->getBaseline()) {
      dustSensor->setBaseline(candidate);
      addLogf("[BASELINE] Adjusted: %.4f", candidate);
    }
    lastBaselineCheck = millis();
  }

  // Send data
  if (millis() - lastSend >= appConfig.sendInterval) {
    latestJson = getDataJson();
    notifyClients(latestJson);

    if (appConfig.mqttEnabled && mqttClient.connected()) {
      if (!mqttClient.publish(appConfig.mqttTopic, latestJson.c_str())) {
        addLogf("[MQTT] Publish FAILED. State=%d", mqttClient.state());
      }
    }

    lastSend = millis();

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
  const char* statusKey = "status_msg";
  const char* uptimeKey = "uptime_seconds";
  const char* rssiKey = "wifi_rssi";


  doc[uptimeKey] = millis() / 1000;
  doc[statusKey] = statusMsg;
  doc[rssiKey] = connected ? WiFi.RSSI() : 0;

  String jsonString;
  serializeJson(doc, jsonString);

  notifyClients(jsonString);
}