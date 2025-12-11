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
#include "calibrate.h"

#include "settings_page.h"
#include "dashboard_page.h"
#include "dashboard_js.h"
#include "ota_update.h"


#include <Update.h>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

bool calibrating = false;


// --- WebSocket ---
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    addLogf("WS: Client #%u connected", client->id());
    client->text(latestJson);
  }
}

void notifyClients(String json) {
  ws.textAll(json);
}

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
    if (strlen(appConfig.wifiSSID) < 1 || !isWifiConnected) {
      request->redirect("/settings");
      return;
    }
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
        addLog("Update failed!");
        response = request->beginResponse(500, "text/plain", "Update failed!");

      } else {
        addLog("Update complete.");
        response = request->beginResponse(200, "text/plain", "Update complete. Rebooting...");
        addLog("Rebooting...");
        delay(1000);
        ESP.restart();
      }
      request->send(response);
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (!index) {
        addLogf("OTA Start: %s\n", filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
      }
      if (len) Update.write(data, len);
      if (final) {
        if (Update.end(true)) addLog("OTA Success!");
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
        addLog("Invalid JSON for OTA URL");
        return;
      }
      const char *url = doc["url"];
      addLogf("OTA from URL: %s\n", url);
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
    doc["mqttEnabled"] = appConfig.mqttEnabled;

    doc["queueMaxSize"] = appConfig.queueMaxSize;
    doc["queueFlushInterval"] = appConfig.queueFlushInterval;


    doc["sendInterval"] = appConfig.sendInterval;
    doc["ntpServer"] = appConfig.ntpServer;
    doc["dustLEDPin"] = appConfig.dustLEDPin;
    doc["dustADCPin"] = appConfig.dustADCPin;
    doc["mqADCPin"] = appConfig.mqADCPin;
    doc["mq_rl_kohm"] = appConfig.mq_rl_kohm;
    doc["mq_r0_ratio_clean"] = appConfig.mq_r0_ratio_clean;

    doc["mq_rzero"] = appConfig.mq_rzero;
    doc["dust_baseline"] = appConfig.dust_baseline;
    doc["dust_calibration"] = appConfig.dust_calibration;

    doc["autoCalibrateOnBoot"] = appConfig.autoCalibrateOnBoot;


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
      if (doc.containsKey("mqttEnabled")) appConfig.mqttEnabled = doc["mqttEnabled"].as<bool>();

      if (doc.containsKey("queueMaxSize")) appConfig.queueMaxSize = doc["queueMaxSize"].as<uint32_t>();
      if (doc.containsKey("queueFlushInterval")) appConfig.queueFlushInterval = doc["queueFlushInterval"].as<uint16_t>();


      if (doc.containsKey("sendInterval")) appConfig.sendInterval = doc["sendInterval"].as<uint32_t>();
      if (doc.containsKey("ntpServer")) strncpy(appConfig.ntpServer, doc["ntpServer"], sizeof(appConfig.ntpServer));
      if (doc.containsKey("mq_rl_kohm")) appConfig.mq_rl_kohm = doc["mq_rl_kohm"].as<float>();
      if (doc.containsKey("mq_r0_ratio_clean")) appConfig.mq_r0_ratio_clean = doc["mq_r0_ratio_clean"].as<float>();
      if (doc.containsKey("mq_rzero")) appConfig.mq_rzero = doc["mq_rzero"].as<float>();
      if (doc.containsKey("dust_baseline")) appConfig.dust_baseline = doc["dust_baseline"].as<float>();
      if (doc.containsKey("dust_calibration")) appConfig.dust_calibration = doc["dust_calibration"].as<float>();

      if (doc.containsKey("autoCalibrateOnBoot")) appConfig.autoCalibrateOnBoot = doc["autoCalibrateOnBoot"].as<bool>();


      if (doc.containsKey("dustLEDPin")) appConfig.dustLEDPin = doc["dustLEDPin"].as<uint8_t>();
      if (doc.containsKey("dustADCPin")) appConfig.dustADCPin = doc["dustADCPin"].as<uint8_t>();
      if (doc.containsKey("mqADCPin")) appConfig.mqADCPin = doc["mqADCPin"].as<uint8_t>();



      saveConfig();
      addLog("Configuration updated. Rebooting...");
      request->send(200, "text/plain", "Settings saved. Rebooting...");
      delay(1000);
      ESP.restart();
    });





  server.on("/reboot", HTTP_GET, handleReboot);
  server.on("/reset", HTTP_GET, handleReset);
  setupCalibrationRoutes();

  server.begin();
  String msg = "Web server started on http://";
  msg += isWifiConnected ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  addLog(msg.c_str());
}