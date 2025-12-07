// weather-esp32.ino
#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ESPAsyncWebServer.h>
#include "config.h"
#include "data.h"


// --- Global Object Definitions ---
// Required for MQTT connection
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Required for WebServer and WebSocket
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Required for data loop timing and state
String latestJson = "{}";
unsigned long lastSend = 0;

// ================= SETUP ==================
void setup() {
  Serial.begin(115200);
  Serial.println("\n--- Starting ESP32 Weather Station Setup ---");


  // 1. Configuration & Sensor Initialization
  loadConfig();  // Load configuration from NVS

  Serial.printf("[CFG] Loaded WiFi: %s | MQTT: %s:%d\n", appConfig.wifiSSID, appConfig.mqttServer, appConfig.mqttPort);

  // Initialize sensors (using config pins)
  initDustSensor();  // Uses appConfig pins
  Serial.println("[SENSOR] GP2Y Dust Sensor initialized.");

  // Initialize BME280
  Serial.print("[SENSOR] Initializing BME280 (0x76)...");
  if (!bme.begin(0x76)) {
    Serial.println("[ERROR] BME280 not found! Check wiring or address.");
    bmeInitialized = false;

  } else {
    Serial.println("BME280 initialized!");
    bmeInitialized = true;
  }

  // Initial GP2Y10 Baseline
  float baseline = dustSensor->getBaselineCandidate();
  dustSensor->setBaseline(baseline);
  Serial.printf("[BASELINE] Initial Dust Baseline set to: %.4f\n", baseline);

  // 2. Connect WiFi (Fallback to Hotspot if failed)
  setupWiFi();

  // 3. NTP Time Synchronization (Continues if failed)
  setupTime();

  // 4. MQTT Configuration
  mqttClient.setServer(appConfig.mqttServer, appConfig.mqttPort);

  // 5. Web Server
  setupWebServer();

  Serial.println("--- Setup Complete. Starting Main Loop ---");
}

// ================= LOOP ==================
void loop() {
  // Maintain WiFi connection (reconnect if lost)
  maintainWiFi();

  // Attempt to maintain MQTT connection if WiFi is active
  reconnectMQTT();
  mqttClient.loop();

  // Handle WebSocket clients
  ws.cleanupClients();

  // Gradual baseline adjustment (only decreases)
  static unsigned long lastBaselineCheck = millis();
  if (millis() - lastBaselineCheck > 60000) {
    float candidate = dustSensor->getBaselineCandidate();
    if (candidate < dustSensor->getBaseline()) {
      dustSensor->setBaseline(candidate);
      Serial.printf("[BASELINE] Adjusted: %.4f\n", candidate);
    }
    lastBaselineCheck = millis();
  }

  // Send data periodically
  if (millis() - lastSend >= appConfig.sendInterval) {
    latestJson = getDataJson();

    // Send via WebSocket (for Dashboard)
    notifyClients(latestJson);

    // Publish via MQTT (only if connected)
    if (mqttClient.connected()) {
      Serial.printf("[MQTT] Publishing to topic: %s\n", appConfig.mqttTopic);

      // DEBUG: in ra gói JSON
      Serial.println("[MQTT] Payload:");
      Serial.println(latestJson);

      // *** ĐÂY LÀ KHỐI LỆNH ĐÃ ĐƯỢC SỬA ĐỂ GHI LOG TRẠNG THÁI PUBLISH ***
      if (mqttClient.publish(appConfig.mqttTopic, latestJson.c_str())) {
        Serial.println("[MQTT] Publish SUCCESS.");
      } else {
        // In ra mã trạng thái lỗi nếu publish thất bại (ví dụ: lỗi kết nối, broker từ chối)
        Serial.printf("[MQTT] Publish FAILED. MQTT State Code: %d\n", mqttClient.state());
      }
      // *** END SỬA ĐỔI ***

    } else if (isWifiConnected) {
      // isWifiConnected cần được cập nhật trong maintainWiFi() hoặc trong loop()
      Serial.println("[MQTT] Not connected. Data not published.");
    }

    lastSend = millis();
  }
}