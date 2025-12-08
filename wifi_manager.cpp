// wifi_manager.cpp

#include <Arduino.h>
#include <WiFi.h>
#include "data.h"
#include "config.h"

bool isWifiConnected = false;
unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = 10000;  // 10s interval

extern PubSubClient mqttClient;
extern AppConfig_t appConfig;

void setupTime() {
  configTime(3 * 3600, 0, appConfig.ntpServer);  // GMT+7
  addLog("[TIME] NTP configured");
}

void startAP() {
  addLog("[AP] Starting Access Point mode...");
  const char* apSSID = "ESP32-Weather-AP";
  const char* apPassword = "12345678";

  WiFi.softAP(apSSID, apPassword);
  addLogf("[AP] AP started. SSID: %s | IP: %s",
          apSSID, WiFi.softAPIP().toString().c_str());

  isWifiConnected = false;
}

void connectToStrongestNode() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);

  addLogf("[WiFi] Scanning for SSID '%s'...", appConfig.wifiSSID);

  int n = WiFi.scanNetworks(false, true);  // include hidden networks, get BSSID
  if (n <= 0) {
    addLog("[WiFi] No networks found");
    startAP();
    return;
  }

  addLogf("[WiFi] Found %d networks:", n);

  // Print all scanned networks
  for (int i = 0; i < n; i++) {
    addLogf(
      "  %02d) SSID=%s | BSSID=%s | RSSI=%d",
      i + 1,
      WiFi.SSID(i).c_str(),
      WiFi.BSSIDstr(i).c_str(),
      WiFi.RSSI(i)
    );
  }

  int32_t bestRSSI = -999;
  String bestBSSIDStr = "";

  // Find strongest BSSID with matching SSID
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) == appConfig.wifiSSID) {
      int32_t rssi = WiFi.RSSI(i);
      String bssid = WiFi.BSSIDstr(i);

      addLogf("[MATCH] SSID=%s | BSSID=%s | RSSI=%d",
              appConfig.wifiSSID,
              bssid.c_str(),
              rssi);

      if (rssi > bestRSSI) {
        bestRSSI = rssi;
        bestBSSIDStr = bssid;
      }
    }
  }

  if (bestBSSIDStr.isEmpty()) {
    addLog("[WiFi] Target SSID not found");
    startAP();
    return;
  }

  // Parse BSSID string → uint8_t array
  uint8_t bestBSSID[6];
  sscanf(bestBSSIDStr.c_str(),
         "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
         &bestBSSID[0], &bestBSSID[1], &bestBSSID[2],
         &bestBSSID[3], &bestBSSID[4], &bestBSSID[5]);

  addLogf("[WiFi] Best match: %s (RSSI %d)",
          bestBSSIDStr.c_str(),
          bestRSSI);

  addLogf("[WiFi] Connecting to BSSID %s...", bestBSSIDStr.c_str());
  WiFi.begin(appConfig.wifiSSID, appConfig.wifiPass, 0, bestBSSID);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    addLog("[WiFi] Connected");
    addLogf("IP: %s", WiFi.localIP().toString().c_str());
    isWifiConnected = true;
  } else {
    addLog("[WiFi] Connection failed");
    startAP();
  }
}


void maintainWiFi() {
  unsigned long now = millis();
  if (now - lastWiFiCheck < wifiCheckInterval) return;
  lastWiFiCheck = now;

  if (WiFi.getMode() & WIFI_AP) {
    addLog("[WiFi] In AP mode, will retry STA later");
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    addLog("[WiFi] Lost connection, reconnecting...");
    connectToStrongestNode();
  }
}

void setupWiFi() {
  connectToStrongestNode();
}

void reconnectMQTT() {
  if (!isWifiConnected) return;
  if (mqttClient.connected()) return;

  addLog("[MQTT] Connecting...");
  String clientId = "ESP32Weather-" + String(random(0xffff), HEX);

  if (mqttClient.connect(clientId.c_str(), appConfig.mqttUser, appConfig.mqttPass)) {
    addLog("[MQTT] Connected");
  } else {
    addLogf("[MQTT] Connection failed, rc=%d", mqttClient.state());
  }
}
