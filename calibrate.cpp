// calibrate.cpp

#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cmath>    // NAN, isfinite
#include <cstdarg>  // va_list

#include "data.h"
#include "config.h"
#include "ota_update.h"
#include "calibrate.h"

extern AsyncWebServer server;
extern AsyncWebSocket ws;

bool calibratingMQ135 = false;
bool calibratingDust = false;

extern bool bmeInitialized;

extern Adafruit_BME280 bme;
extern GP2YDustSensor *dustSensor;
extern MQ135 *mq135;

extern AppConfig_t appConfig;

void applyConfigToSensors() {
  if (mq135 && isfinite(appConfig.mq_rzero)) {
    mq135->setRZero(appConfig.mq_rzero);
    addLogf("[MQ135] Reloaded new RZERO=%.3f", appConfig.mq_rzero);
  }

  if (dustSensor && isfinite(appConfig.dust_baseline)) {
    dustSensor->setBaseline(appConfig.dust_baseline);
    addLogf("[Dust] Reloaded new baseline=%.4f", appConfig.dust_baseline);
  }
}

void startCalibration() {
  if (calibratingMQ135 || calibratingDust) {
    addLog("Calibration already running");
    return;
  }

  calibratingMQ135 = true;
  calibratingDust = true;

  addLog("Calibration started...");

  // MQ135
  xTaskCreate([](void *param) {
    bool *pCalib = (bool *)param;
    float temp = NAN, hum = NAN;
    if (bmeInitialized) {
      temp = bme.readTemperature();
      hum = bme.readHumidity();
    }
    if (!isfinite(temp) || !isfinite(hum)) {
      addLog("BME280 not ready");
      *pCalib = false;
      vTaskDelete(NULL);
      return;
    }
    float lastR0 = NAN;
    for (int i = 1; i <= 50; i++) {
      float r0 = mq135->autoCalibrate(temp, hum);
      if (isfinite(r0)) lastR0 = r0;
      addLogf("MQ135 round %d: RZERO=%.3f", i, r0);
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    if (isfinite(lastR0)) {
      appConfig.mq_rzero = lastR0;
      saveConfig();
      addLogf("MQ135 calibration finished. Final RZERO=%.3f", lastR0);
      applyConfigToSensors();
    }
    *pCalib = false;
    vTaskDelete(NULL);
  },
              "MQ135CalTask", 4096, &calibratingMQ135, 1, NULL);

  // Dust
  xTaskCreate([](void *param) {
    bool *pCalib = (bool *)param;

    if (!dustSensor) {
      addLog("Dust sensor not initialized");
      *pCalib = false;
      vTaskDelete(NULL);
      return;
    }

    float lastBaseline = NAN;

    for (int i = 1; i <= 50; i++) {
      float candidate = dustSensor->getBaselineCandidate();
      if (isfinite(candidate)) lastBaseline = candidate;

      addLogf("Dust round %d: Baseline=%.4f", i, candidate);
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    if (isfinite(lastBaseline)) {
      dustSensor->setBaseline(lastBaseline);

      // --- SAVE TO CONFIG ---
      appConfig.dust_baseline = lastBaseline;
      saveConfig();

      addLogf("Dust calibration finished. Final baseline=%.4f (saved)", lastBaseline);
      applyConfigToSensors();
    }

    *pCalib = false;
    vTaskDelete(NULL);
  },
              "DustCalTask", 4096, &calibratingDust, 1, NULL);
}


void setupCalibrationRoutes() {
  // Web page chung
  server.on("/calibrate", HTTP_GET, [](AsyncWebServerRequest *request) {
    String page = R"HTML(
<html>
<body style='font-family:Arial;padding:20px;'>
<h2>Calibrate MQ135 & Dust Sensor</h2>
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
function startCalibrate(){ fetch('/api/calibrate'); document.getElementById('log').innerText="Calibration started...\n"; }
</script>
</body>
</html>)HTML";
    request->send(200, "text/html", page);
  });

  // API chung chạy đồng thời MQ135 và Dust
  server.on("/api/calibrate", [](AsyncWebServerRequest *request) {
    startCalibration();
    request->send(200, "text/plain", "Calibration started...");
  });
}
