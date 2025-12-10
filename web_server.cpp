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
    doc["mq_rzero"] = appConfig.mq_rzero;


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
      if (doc.containsKey("mq_rl_kohm")) appConfig.mq_rl_kohm = doc["mq_rl_kohm"].as<float>();
      if (doc.containsKey("mq_r0_ratio_clean")) appConfig.mq_r0_ratio_clean = doc["mq_r0_ratio_clean"].as<float>();
      if (doc.containsKey("mq_rzero")) appConfig.mq_rzero = doc["mq_rzero"].as<float>();
      if (doc.containsKey("dustLEDPin")) appConfig.dustLEDPin = doc["dustLEDPin"].as<uint8_t>();
      if (doc.containsKey("dustADCPin")) appConfig.dustADCPin = doc["dustADCPin"].as<uint8_t>();
      if (doc.containsKey("mqADCPin")) appConfig.mqADCPin = doc["mqADCPin"].as<uint8_t>();


      saveConfig();
      addLog("Configuration updated. Rebooting...");
      request->send(200, "text/plain", "Settings saved. Rebooting...");
      delay(1000);
      ESP.restart();
    });



  server.on("/calibrate-mq135", HTTP_GET, [](AsyncWebServerRequest *request) {
    String page = R"HTML(
<html>
<body style='font-family:Arial;padding:20px;'>
<h2>Calibrate MQ135</h2>
<button onclick="startCalibrate()" style='padding:10px;font-size:18px;'>Start Calibration</button>
<pre id='log' style='background:#000;color:#0f0;padding:10px;margin-top:20px;height:200px;overflow:auto;white-space: pre-wrap;'></pre>
<script>
let ws = new WebSocket("ws://" + location.host + "/ws");
ws.onmessage = (event) => {
    try {
        let msg = JSON.parse(event.data);
         if(msg.type === "log"){
            let logEl = document.getElementById('log');
            logEl.innerHTML += msg.msg + "\n";  
            logEl.scrollTop = logEl.scrollHeight;
        }
    } catch(e){}
};

function startCalibrate(){
    document.getElementById('log').innerText = "Calibration started...\n";
    fetch('/api/mq135/calibrate');
}
</script>
</body>
</html>
    )HTML";

    request->send(200, "text/html", page);
  });


  server.on("/api/mq135/calibrate", [&](AsyncWebServerRequest *request) {
    if (calibrating) {
      request->send(400, "text/plain", "Calibration already running");
      return;
    }

    calibrating = true;
    request->send(200, "text/plain", "Calibration started...");

    // truyền địa chỉ biến vào task
    xTaskCreate([](void *param) {
      bool *pCalibrating = (bool *)param;

      float temp = NAN, hum = NAN;
      if (bmeInitialized) {
        temp = bme.readTemperature();
        hum = bme.readHumidity();
      }

      if (!isfinite(temp) || !isfinite(hum)) {
        addLog("BME280 not ready");
        *pCalibrating = false;
        vTaskDelete(NULL);
        return;
      }

      const int rounds = 50;
      float lastR0 = NAN;

      for (int i = 1; i <= rounds; i++) {
        float r0 = mq135->autoCalibrate(temp, hum);
        if (isfinite(r0)) {
          lastR0 = r0;
          addLogf("Calibration round %d: RZERO=%.3f", i, r0);
        } else {
          addLogf("Calibration round %d failed", i);
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
      }

      if (isfinite(lastR0)) {
        appConfig.mq_rzero = lastR0;
        saveConfig();
        addLogf("Calibration finished. Final RZERO=%.3f", lastR0);
      } else {
        addLog("Calibration failed");
      }

      *pCalibrating = false;
      vTaskDelete(NULL);
    },
                "MQ135CalTask", 4096, &calibrating, 1, NULL);
  });



  server.on("/reboot", HTTP_GET, handleReboot);
  server.on("/reset", HTTP_GET, handleReset);

  server.begin();
  String msg = "Web server started on http://";
  msg += isWifiConnected ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  addLog(msg.c_str());
}