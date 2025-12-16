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
#include <HTTPClient.h> // NEW: Include HTTPClient for downloading firmware

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

bool calibrating = false;

// NEW: Global variable to hold the URL for OTA update
String g_ota_url = "";


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


// --- OTA from URL Logic ---
void startOtaFromUrl(const char *url) {
    addLogf("OTA URL: Starting update from %s", url);

    // Use WiFiClient for HTTP
    WiFiClient client;
    HTTPClient http;

    if (!http.begin(client, url)) {
        addLog("HTTP: Failed to connect or invalid URL.");
        return;
    }

    http.addHeader("User-Agent", "ESP32-OTA-Agent/1.0");
    http.addHeader("Accept", "*/*");

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        addLogf("HTTP: GET failed. Code: %d", httpCode);
        http.end();
        return;
    }

    int contentLength = http.getSize();
    if (contentLength <= 0) {
        addLog("HTTP: Invalid content length or file size.");
        http.end();
        return;
    }

    addLogf("Firmware size: %d bytes", contentLength);

    if (!Update.begin(contentLength)) {
        addLog("Update: Failed to begin update.");
        Update.printError(Serial);
        http.end();
        return;
    }

    // === Robust Download and Write Logic ===

    WiFiClient *stream = http.getStreamPtr();
    size_t written = 0;
    size_t totalWritten = 0;

    // Tải xuống theo từng khối 1024 bytes
    uint8_t buff[1024] = {0};
    int bytesRead = 0;

    // Lặp cho đến khi hết luồng
    while (stream->connected() && (totalWritten < contentLength || contentLength == -1)) {
        // Đọc dữ liệu nếu có sẵn
        bytesRead = stream->readBytes(buff, sizeof(buff));

        if (bytesRead > 0) {
            // Ghi dữ liệu vào Update
            written = Update.write(buff, bytesRead);
            totalWritten += written;

            // Log tiến trình mỗi 5% (tùy chọn)
            static int lastProgress = 0;
            int progress = (totalWritten * 100) / contentLength;
            if (progress - lastProgress >= 5) {
                addLogf("OTA Progress: %d%% (%u bytes)", progress, totalWritten);
                lastProgress = progress;
            }

            if (written != bytesRead) {
                // Lỗi ghi vào bộ nhớ flash
                addLog("Update write error!");
                break;
            }
        } else if (bytesRead == -1) {
            // Lỗi đọc stream
            addLog("Stream read error!");
            break;
        }

        // Dừng nếu đạt đến ContentLength và đã đọc xong
        if (contentLength != -1 && totalWritten >= contentLength) break;
    }

    // Kiểm tra kết quả
    if (totalWritten == contentLength) {
        addLogf("Update: Successfully wrote %u bytes.", totalWritten);
    } else {
        addLogf("Update: Final check failed. Wrote %u/%u bytes.", totalWritten, contentLength);
    }

    http.end(); // Close the HTTP connection

    if (Update.end(true)) {
        addLog("Update Success. Rebooting...");
        delay(1000);
        ESP.restart();
    } else {
        addLog("Update failed!");
        Update.printError(Serial);
    }
}

// Hàm này cần được gọi trong loop() để kích hoạt OTA
void handleOtaUrlInLoop() {
  if (g_ota_url.length() > 0) {
    String urlCopy = g_ota_url;
    g_ota_url = ""; // Clear the flag immediately

    // Execute the OTA update (this will be blocking)
    startOtaFromUrl(urlCopy.c_str());
  }
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
      // Send quick response to prevent browser timeout
      request->send(200, "text/plain", "OTA process initiated. Check log for details.");
    },
    NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      // Parse the JSON body to get the URL
      StaticJsonDocument<256> doc;
      DeserializationError err = deserializeJson(doc, data, len);
      if (err) {
        addLog("Invalid JSON for OTA URL");
        return;
      }
      const char *url = doc["url"];

      // Store the URL globally to be processed in the main loop()
      if (url && strlen(url) > 0) {
          g_ota_url = String(url);
          addLogf("OTA URL received: %s. Starting update shortly...", url);
      } else {
          addLog("OTA URL received was empty.");
      }
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
    StaticJsonDocument<768> doc;

    // I. GENERAL & DEVICE INFO
    doc["deviceId"] = appConfig.deviceId;
    doc["latitude"] = appConfig.latitude;
    doc["longitude"] = appConfig.longitude;
    doc["stationName"] = appConfig.stationName;
    doc["stationDescription"] = appConfig.stationDescription;

    // II. NETWORK & WIFI
    doc["wifiSSID"] = appConfig.wifiSSID;
    doc["wifiPass"] = appConfig.wifiPass;

    // III. MQTT & DATA SERVICE
    doc["mqttEnabled"] = appConfig.mqttEnabled;
    doc["mqttServer"] = appConfig.mqttServer;
    doc["mqttPort"] = appConfig.mqttPort;
    // doc["mqttUser"] = appConfig.mqttUser; // Removed
    doc["mqttPass"] = appConfig.mqttPass;

    doc["queueMaxSize"] = appConfig.queueMaxSize;
    doc["queueFlushInterval"] = appConfig.queueFlushInterval;

    // IV. TIMING & POWER
    doc["sendInterval"] = appConfig.sendInterval;
    doc["ntpServer"] = appConfig.ntpServer;
    doc["timeZone"] = appConfig.timeZone;
    doc["enableSleep"] = appConfig.enableSleep;
    doc["sleepDuration"] = appConfig.sleepDuration;


    // V. SENSOR PINS & CONFIG
    doc["dustLEDPin"] = appConfig.dustLEDPin;
    doc["dustADCPin"] = appConfig.dustADCPin;
    doc["mqADCPin"] = appConfig.mqADCPin;

    doc["pmsEnabled"] = appConfig.pmsEnabled;
    doc["pmsRxPin"] = appConfig.pmsRxPin;
    doc["pmsTxPin"] = appConfig.pmsTxPin;
    doc["pmsSetPin"] = appConfig.pmsSetPin;


    // VI. SENSOR CALIBRATION & OFFSET
    doc["autoCalibrateOnBoot"] = appConfig.autoCalibrateOnBoot;

    doc["mq_rl_kohm"] = appConfig.mq_rl_kohm;
    doc["mq_r0_ratio_clean"] = appConfig.mq_r0_ratio_clean;
    doc["mq_rzero"] = appConfig.mq_rzero; // Persisted state (Read-only on form)

    doc["dust_baseline"] = appConfig.dust_baseline; // Persisted state (Read-only on form)
    doc["dust_calibration"] = appConfig.dust_calibration;


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
      StaticJsonDocument<768> doc;
      DeserializationError error = deserializeJson(doc, (const char *)data, len);
      if (error) {
        request->send(400, "text/plain", "Invalid JSON");
        return;
      }

      // I. GENERAL & DEVICE INFO
	  if (doc.containsKey("deviceId")) appConfig.deviceId = doc["deviceId"].as<uint32_t>();
      if (doc.containsKey("latitude")) appConfig.latitude = doc["latitude"].as<float>();
      if (doc.containsKey("longitude")) appConfig.longitude = doc["longitude"].as<float>();

      if (doc.containsKey("stationName")) strncpy(appConfig.stationName, doc["stationName"], sizeof(appConfig.stationName));
      if (doc.containsKey("stationDescription")) strncpy(appConfig.stationDescription, doc["stationDescription"], sizeof(appConfig.stationDescription));

      // II. NETWORK & WIFI
      if (doc.containsKey("wifiSSID")) strncpy(appConfig.wifiSSID, doc["wifiSSID"], sizeof(appConfig.wifiSSID));
      if (doc.containsKey("wifiPass")) strncpy(appConfig.wifiPass, doc["wifiPass"], sizeof(appConfig.wifiPass));

      // III. MQTT & DATA SERVICE
      if (doc.containsKey("mqttEnabled")) appConfig.mqttEnabled = doc["mqttEnabled"].as<bool>();
      if (doc.containsKey("mqttServer")) strncpy(appConfig.mqttServer, doc["mqttServer"], sizeof(appConfig.mqttServer));
      if (doc.containsKey("mqttPort")) appConfig.mqttPort = doc["mqttPort"].as<uint16_t>();
      // if (doc.containsKey("mqttUser")) strncpy(appConfig.mqttUser, doc["mqttUser"], sizeof(appConfig.mqttUser)); // Removed
      if (doc.containsKey("mqttPass")) strncpy(appConfig.mqttPass, doc["mqttPass"], sizeof(appConfig.mqttPass));
      if (doc.containsKey("queueMaxSize")) appConfig.queueMaxSize = doc["queueMaxSize"].as<uint32_t>();
      if (doc.containsKey("queueFlushInterval")) appConfig.queueFlushInterval = doc["queueFlushInterval"].as<uint16_t>();


      // IV. TIMING & POWER
      if (doc.containsKey("sendInterval")) appConfig.sendInterval = doc["sendInterval"].as<uint32_t>();
      if (doc.containsKey("ntpServer")) strncpy(appConfig.ntpServer, doc["ntpServer"], sizeof(appConfig.ntpServer));
      if (doc.containsKey("timeZone")) appConfig.timeZone = doc["timeZone"].as<int>();
      if (doc.containsKey("enableSleep")) appConfig.enableSleep = doc["enableSleep"].as<bool>();
      if (doc.containsKey("sleepDuration")) appConfig.sleepDuration = doc["sleepDuration"].as<uint32_t>();


      // V. SENSOR PINS & CONFIG
      if (doc.containsKey("dustLEDPin")) appConfig.dustLEDPin = doc["dustLEDPin"].as<uint8_t>();
      if (doc.containsKey("dustADCPin")) appConfig.dustADCPin = doc["dustADCPin"].as<uint8_t>();
      if (doc.containsKey("mqADCPin")) appConfig.mqADCPin = doc["mqADCPin"].as<uint8_t>();

      if (doc.containsKey("pmsEnabled")) appConfig.pmsEnabled = doc["pmsEnabled"].as<bool>();
      if (doc.containsKey("pmsRxPin")) appConfig.pmsRxPin = doc["pmsRxPin"].as<uint8_t>();
      if (doc.containsKey("pmsTxPin")) appConfig.pmsTxPin = doc["pmsTxPin"].as<uint8_t>();
      if (doc.containsKey("pmsSetPin")) appConfig.pmsSetPin = doc["pmsSetPin"].as<int>();


      // VI. SENSOR CALIBRATION & OFFSET
      if (doc.containsKey("autoCalibrateOnBoot")) appConfig.autoCalibrateOnBoot = doc["autoCalibrateOnBoot"].as<bool>();

      if (doc.containsKey("mq_rl_kohm")) appConfig.mq_rl_kohm = doc["mq_rl_kohm"].as<float>();
      if (doc.containsKey("mq_r0_ratio_clean")) appConfig.mq_r0_ratio_clean = doc["mq_r0_ratio_clean"].as<float>();
      // mq_rzero is persisted, not usually changed from form

      if (doc.containsKey("dust_calibration")) appConfig.dust_calibration = doc["dust_calibration"].as<float>();
      // dust_baseline is persisted, not usually changed from form


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