//web_server.cpp
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <cstdint>
#include <cstring>

#include "config.h"  // appConfig
#include "data.h"    // ws, server, onWsEvent, isWifiConnected

#include "settings_page.h"
#include "dashboard_page.h"
#include "dashboard_js.h"

#include <Update.h>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ======== Reboot / Reset handlers ========
void handleReboot(AsyncWebServerRequest *request) {
  request->send(200, "text/html", "<html><body>Rebooting...</body></html>");
  addLog("Manual reboot requested. Restarting...");
  delay(1000);
  ESP.restart();
}

void handleReset(AsyncWebServerRequest *request) {
  request->send(200, "text/html", "<html><body>Factory reset. Rebooting...</body></html>");
  addLog("Factory reset requested. Erasing NVS...");
  resetConfig();
  delay(1000);
  ESP.restart();
}

// ======== Send data to WebSocket ========
void sendWSData(const char *json) {
  ws.textAll(json);
}

// ======== Setup server ========
void setupWebServer() {
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // Dashboard
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = getDashboardPageHTML();
    request->send(200, "text/html; charset=utf-8", html);
  });

  // OTA page
  server.on("/ota", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", getOTAPageHTML("Ready for OTA update"));
  });

  // Upload file
  server.on(
    "/update-file", HTTP_POST, [](AsyncWebServerRequest *request) {
      AsyncWebServerResponse *response;
      if (Update.hasError()) {
        response = request->beginResponse(500, "text/plain", "Update failed!");
      } else {
        response = request->beginResponse(200, "text/plain", "Update complete. Rebooting...");
        ESP.restart();
      }
      request->send(response);
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (!index) {
        Serial.printf("OTA Start: %s\n", filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
      }
      if (len) Update.write(data, len);
      if (final) {
        if (Update.end(true)) Serial.println("OTA Success!");
        else Update.printError(Serial);
      }
    });

  // Update from URL
  server.on(
    "/update-url", HTTP_POST, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", "Starting OTA from URL...");
    },
    NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<256> doc;
      DeserializationError err = deserializeJson(doc, data, len);
      if (err) {
        Serial.println("Invalid JSON for OTA URL");
        return;
      }
      const char *url = doc["url"];
      Serial.printf("OTA from URL: %s\n", url);
      // Ở đây bạn cần gọi hàm HTTP client download và Update.writeStream()
    });

  server.on("/dashboard.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(
      200,
      "application/javascript",
      reinterpret_cast<const uint8_t *>(dashboard_js),
      strlen(dashboard_js));
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
  });


  // Settings
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    StaticJsonDocument<512> doc;
    doc["deviceId"] = appConfig.deviceId;
    doc["latitude"] = appConfig.latitude;
    doc["longitude"] = appConfig.longitude;
    doc["wifiSSID"] = appConfig.wifiSSID;
    doc["wifiPass"] = appConfig.wifiPass;
    doc["mqttServer"] = appConfig.mqttServer;
    doc["mqttPort"] = appConfig.mqttPort;
    doc["mqttUser"] = appConfig.mqttUser;
    doc["mqttPass"] = appConfig.mqttPass;
    doc["mqttTopic"] = appConfig.mqttTopic;
    doc["sendInterval"] = appConfig.sendInterval;
    doc["ntpServer"] = appConfig.ntpServer;
    doc["dustLEDPin"] = appConfig.dustLEDPin;
    doc["dustADCPin"] = appConfig.dustADCPin;
    doc["mqADCPin"] = appConfig.mqADCPin;
    doc["mq_rl_kohm"] = appConfig.mq_rl_kohm;
    doc["mq_r0_ratio_clean"] = appConfig.mq_r0_ratio_clean;
    doc["tvoc_a_curve"] = appConfig.tvoc_a_curve;
    doc["tvoc_b_curve"] = appConfig.tvoc_b_curve;

    String jsonConfig;
    serializeJson(doc, jsonConfig);

    String statusColor = isWifiConnected ? "green" : "red";
    String statusIP = isWifiConnected ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    String statusMsg = "<span style='color:" + statusColor + ";'>" + (isWifiConnected ? "Connected: " : "Hotspot Mode: ") + statusIP + "</span>";

    request->send(200, "text/html; charset=utf-8", getSettingsPageHTML(statusMsg, jsonConfig));
  });


  // Save config
  server.on(
    "/save", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, (const char *)data, len);
      if (error) {
        request->send(400, "text/plain", "Invalid JSON");
        return;
      }

      if (doc.containsKey("deviceId")) strncpy(appConfig.deviceId, doc["deviceId"], sizeof(appConfig.deviceId));
      if (doc.containsKey("latitude")) appConfig.latitude = doc["latitude"].as<float>();
      if (doc.containsKey("longitude")) appConfig.longitude = doc["longitude"].as<float>();
      if (doc.containsKey("wifiSSID")) strncpy(appConfig.wifiSSID, doc["wifiSSID"], sizeof(appConfig.wifiSSID));
      if (doc.containsKey("wifiPass")) strncpy(appConfig.wifiPass, doc["wifiPass"], sizeof(appConfig.wifiPass));
      if (doc.containsKey("mqttServer")) strncpy(appConfig.mqttServer, doc["mqttServer"], sizeof(appConfig.mqttServer));
      if (doc.containsKey("mqttPort")) appConfig.mqttPort = doc["mqttPort"].as<uint16_t>();
      if (doc.containsKey("mqttUser")) strncpy(appConfig.mqttUser, doc["mqttUser"], sizeof(appConfig.mqttUser));
      if (doc.containsKey("mqttPass")) strncpy(appConfig.mqttPass, doc["mqttPass"], sizeof(appConfig.mqttPass));
      if (doc.containsKey("mqttTopic")) strncpy(appConfig.mqttTopic, doc["mqttTopic"], sizeof(appConfig.mqttTopic));
      if (doc.containsKey("sendInterval")) appConfig.sendInterval = doc["sendInterval"].as<uint32_t>();
      if (doc.containsKey("ntpServer")) strncpy(appConfig.ntpServer, doc["ntpServer"], sizeof(appConfig.ntpServer));
      saveConfig();
      addLog("Configuration updated. Rebooting...");
      request->send(200, "text/plain", "Settings saved. Rebooting...");
      delay(1000);
      ESP.restart();
    });

  server.on("/reboot", HTTP_GET, handleReboot);
  server.on("/reset", HTTP_GET, handleReset);

  server.begin();
  String msg = "Web server started on http://";
  msg += isWifiConnected ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  addLog(msg.c_str());
}