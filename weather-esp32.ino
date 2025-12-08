#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ESPAsyncWebServer.h>
#include "config.h"
#include "data.h"
#include "MQ135_ESP32.h"

// --- Global Objects ---
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

String latestJson = "{}";
unsigned long lastSend = 0;
void logAppConfig() {
  addLog("===== AppConfig =====");

  // WiFi
  addLogf("WiFi SSID       : %s", appConfig.wifiSSID);
  addLogf("WiFi Pass       : %s", appConfig.wifiPass);

  // MQTT
  addLogf("MQTT Server     : %s", appConfig.mqttServer);
  addLogf("MQTT Port       : %d", appConfig.mqttPort);
  addLogf("MQTT User       : %s", appConfig.mqttUser);
  addLogf("MQTT Pass       : %s", appConfig.mqttPass);
  addLogf("MQTT Topic      : %s", appConfig.mqttTopic);

  // Time
  addLogf("NTP Server      : %s", appConfig.ntpServer);
  addLogf("Send Interval   : %lu ms", appConfig.sendInterval);

  // GPIO
  addLogf("Dust LED Pin    : %d", appConfig.dustLEDPin);
  addLogf("Dust ADC Pin    : %d", appConfig.dustADCPin);
  addLogf("MQ135 ADC Pin   : %d", appConfig.mqADCPin);

  // MQ135 calibration
  addLog("---- MQ135 Calibration ----");
  addLogf("RL (kOhm)       : %.3f", appConfig.mq_rl_kohm);
  addLogf("R0 Clean Ratio  : %.3f", appConfig.mq_r0_ratio_clean);
  addLogf("TVOC A Curve    : %.3f", appConfig.tvoc_a_curve);
  addLogf("TVOC B Curve    : %.3f", appConfig.tvoc_b_curve);
  addLogf("RZero           : %.3f", appConfig.mq_rzero);
  addLogf("CO2 A Curve     : %.3f", appConfig.co2_a_curve);
  addLogf("CO2 B Curve     : %.3f", appConfig.co2_b_curve);
  addLogf("NH3 A Curve     : %.3f", appConfig.nh3_a_curve);
  addLogf("NH3 B Curve     : %.3f", appConfig.nh3_b_curve);
  addLogf("Alcohol A Curve : %.3f", appConfig.alcohol_a_curve);
  addLogf("Alcohol B Curve : %.3f", appConfig.alcohol_b_curve);

  // Device info
  addLog("---- Device Info ----");
  addLogf("Device ID       : %s", appConfig.deviceId);
  addLogf("Latitude        : %.6f", appConfig.latitude);
  addLogf("Longitude       : %.6f", appConfig.longitude);

  addLog("======================");
}


void setup() {
  Serial.begin(115200);
  addLog("--- Starting ESP32 Weather Station ---");

  loadConfig();
  logAppConfig();

  addLog("[INIT] WiFi...");
  setupWiFi();

  addLog("[INIT] Web server...");
  setupWebServer();

  addLog("[INIT] Dust sensor...");
  initDustSensor();

  addLog("[INIT] MQ135 sensor...");
  initMQ135();

  addLog("[INIT] BME280...");
  if (!bme.begin(0x76)) {
    addLog("[ERROR] BME280 not found");
    bmeInitialized = false;
  } else {
    addLog("BME280 initialized");
    bmeInitialized = true;
  }

  float baseline = dustSensor->getBaselineCandidate();
  dustSensor->setBaseline(baseline);
  addLogf("[BASELINE] Dust baseline set to %.4f", baseline);

  addLog("[MQ135] Warmup...");


  addLog("[INIT] Time sync...");
  setupTime();

  addLogf("[INIT] MQTT server: %s:%d", appConfig.mqttServer, appConfig.mqttPort);
  mqttClient.setServer(appConfig.mqttServer, appConfig.mqttPort);

  addLog("--- Setup Complete ---");
}

void loop() {
  maintainWiFi();
  reconnectMQTT();
  mqttClient.loop();
  ws.cleanupClients();

  // Dust baseline adjustment
  static unsigned long lastBaselineCheck = 0;
  if (millis() - lastBaselineCheck > 60000) {
    float candidate = dustSensor->getBaselineCandidate();
    if (candidate < dustSensor->getBaseline()) {
      dustSensor->setBaseline(candidate);
      addLogf("[BASELINE] Adjusted: %.4f", candidate);
    }
    lastBaselineCheck = millis();
  }

  mq135->warmupLoop();

  if (!mq135->isWarmingUp()) {
    processMQ135Calibration(*mq135);
  }

  // Send data
  if (millis() - lastSend >= appConfig.sendInterval) {
    latestJson = getDataJson();
    notifyClients(latestJson);

    if (mqttClient.connected()) {
      if (!mqttClient.publish(appConfig.mqttTopic, latestJson.c_str())) {
        addLogf("[MQTT] Publish FAILED. State=%d", mqttClient.state());
      }
    } else if (isWifiConnected) {
      addLog("[MQTT] Not connected. Data not published.");
    }

    lastSend = millis();
  }
}
