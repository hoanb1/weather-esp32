// wifi_manager.cpp

#include <Arduino.h>
#include <WiFi.h>
#include "data.h"
#include "config.h"



// Global Variables
bool isWifiConnected = false;
unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = 10000;  // Check WiFi status every 10s



extern PubSubClient mqttClient;
extern AppConfig_t appConfig;

// Function to set up Time (Needs to be defined)
void setupTime() {
  configTime(3 * 3600, 0, appConfig.ntpServer);  // 3 * 3600 for GMT+7
  Serial.println("[TIME] NTP time configured.");
}

// Start AP fallback
/**
 * @brief Starts the ESP32 in Access Point (AP) mode for configuration.
 */
void startAP() {
  Serial.println("[AP] Starting Access Point mode...");
  const char *apSSID = "ESP32-Weather-AP";
  const char *apPassword = "12345678";

  WiFi.softAP(apSSID, apPassword);
  Serial.printf("[AP] AP started! SSID: %s | IP: %s\n", apSSID, WiFi.softAPIP().toString().c_str());
  isWifiConnected = false;
}

/**
 * @brief Scans for the configured SSID and connects to the strongest available BSSID/Node.
 */
void connectToStrongestNode() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);

  Serial.printf("[WiFi] Scanning for SSID '%s'...\n", appConfig.wifiSSID);
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("[WiFi] No networks found!");
    startAP();
    return;
  }

  int32_t bestRSSI = -999;
  String bestBSSIDStr;

  // Find the strongest BSSID/Node for the configured SSID
  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    int32_t rssi = WiFi.RSSI(i);
    String bssid = WiFi.BSSIDstr(i);

    if (ssid == appConfig.wifiSSID && rssi > bestRSSI) {
      bestRSSI = rssi;
      bestBSSIDStr = bssid;
    }
  }

  if (bestBSSIDStr.length() == 0) {
    // Original: Serial.println("[WiFi] SSID not found! Starting AP...");
    Serial.println("[WiFi] SSID not found! Starting AP..."); 
    startAP();
    return;
  }

  // Convert BSSID string -> uint8_t[6]
  uint8_t bestBSSID[6];
  sscanf(bestBSSIDStr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
         &bestBSSID[0], &bestBSSID[1], &bestBSSID[2],
         &bestBSSID[3], &bestBSSID[4], &bestBSSID[5]);

  Serial.printf("[WiFi] Connecting to BSSID %s with RSSI %d...\n", bestBSSIDStr.c_str(), bestRSSI);
  // Connect using BSSID for stability
  WiFi.begin(appConfig.wifiSSID, appConfig.wifiPass, 0, bestBSSID);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {  // 30*500ms = 15s timeout
    delay(500);
    //Serial.println(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Connected!");
    Serial.printf("    - Local IP: %s\n", WiFi.localIP().toString().c_str());
    isWifiConnected = true;
  } else {
    // Original: Serial.println("\n[WiFi] Connection failed! Starting AP...");
    Serial.println("\n[WiFi] Connection failed! Starting AP..."); 
    startAP();
  }
}

/**
 * @brief Called periodically in the loop to check WiFi status and reconnect if disconnected.
 */
void maintainWiFi() {
  // --- 1. Throttling Check (10s interval) ---
  unsigned long now = millis();
  if (now - lastWiFiCheck < wifiCheckInterval) return;
  lastWiFiCheck = now;

  // --- 2. Check current mode and status ---
  
  // A. Check if the device is currently in Access Point mode (Hotspot)
  if (WiFi.getMode() & WIFI_AP) {
      // Logic: If currently in AP, we won't try to switch back to STA immediately.
      // If we don't want to do anything and just hold AP, return:
      Serial.println("[WiFi] Currently in AP mode. Retrying STA later.");
      return; 
  }
  
  // B. If the device is in Station mode (STA) and lost connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Lost connection, trying to reconnect...");
    // Re-run the full scan and connect logic
    connectToStrongestNode();
  }
}

/**
 * @brief Main function called in setup() to establish the initial WiFi connection.
 */
void setupWiFi() {
  connectToStrongestNode();
}

/**
 * @brief Attempts to connect to the MQTT broker.
 */
void reconnectMQTT() {
  if (!isWifiConnected) return;  // Only attempt MQTT connection if WiFi is up

  if (mqttClient.connected()) return;

  Serial.print("[MQTT] Attempting connection...");
  // Generate unique client ID for MQTT
  String clientId = "ESP32Weather-" + String(random(0xffff), HEX);

  // Attempt to connect with username and password
  if (mqttClient.connect(clientId.c_str(), appConfig.mqttUser, appConfig.mqttPass)) {
    Serial.println("connected!");
  } else {
    // Print error code (rc)
    Serial.printf("failed, rc=%d. Retrying in 5s...\n", mqttClient.state());
    // No delay here, retrying should happen naturally in the loop()
  }
}