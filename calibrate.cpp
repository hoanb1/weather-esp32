// File: calibrate.cpp
#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cmath>
#include <cstdarg>

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

unsigned long lastBaselineCalc = 0;

#define BASELINE_CALC_INTERVAL 60000
#define LAST_N_R0 8
#define LAST_N_DUST 8

extern AppConfig_t appConfig;

// ---------------- Apply config to sensors ----------------
void applyConfigToSensors() {
    if (mq135 && isfinite(appConfig.mq_rzero)) {
        mq135->setRZero(appConfig.mq_rzero);
        addLogf("Applied MQ135 R0 = %.3f", appConfig.mq_rzero);
    }
    if (dustSensor && isfinite(appConfig.dust_baseline)) {
        dustSensor->setBaseline(appConfig.dust_baseline);
        addLogf("Applied Dust Baseline = %.3f", appConfig.dust_baseline);
    }
}

// ---------------- Auto drift correction ----------------
void updateBaselineDriftCorrection() {
    if (millis() - lastBaselineCalc < BASELINE_CALC_INTERVAL) return;

    float newCandidate = dustSensor->getBaselineCandidate();
    float oldBaseline = appConfig.dust_baseline;
    float saveValue = oldBaseline;

    if (newCandidate > 0.0 && newCandidate < dustSensor->getBaseline()) {
        saveValue = newCandidate;
        dustSensor->setBaseline(saveValue);

        if (saveValue != oldBaseline) {
            appConfig.dust_baseline = saveValue;
            saveConfig();
            addLogf("Dust baseline drift updated: %.3f -> %.3f", oldBaseline, saveValue);
        }
    }

    lastBaselineCalc = millis();
}

// ---------------- MQ135 Calibration Task ----------------
void mq135CalibrationTask(void *param) {
    bool *pCalib = (bool *)param;
    float temp = NAN, hum = NAN;

    if (bmeInitialized) {
        temp = bme.readTemperature();
        hum = bme.readHumidity();
    }

    if (!isfinite(temp) || !isfinite(hum)) {
        addLog("MQ135 calibration aborted: invalid temperature/humidity");
        *pCalib = false;
        vTaskDelete(NULL);
        return;
    }

    addLogf("MQ135 calibration started (T=%.2f, H=%.2f)", temp, hum);

    float r0Values[20];
    int validCount = 0;

    for (int i = 1; i <= 20; i++) {
        float r0 = mq135->autoCalibrate(temp, hum);
        if (isfinite(r0)) {
            r0Values[validCount++] = r0;
            addLogf("MQ135 sample %d: R0 = %.3f", i, r0);
        } else {
            addLogf("MQ135 sample %d: invalid", i);
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    if (validCount == 0) {
        addLog("MQ135 calibration failed: no valid R0");
        *pCalib = false;
        vTaskDelete(NULL);
        return;
    }

    int N = min(LAST_N_R0, validCount);
    float sum = 0;
    for (int i = validCount - N; i < validCount; i++) sum += r0Values[i];

    float avgR0 = sum / N;
    float oldR0 = appConfig.mq_rzero;

    appConfig.mq_rzero = avgR0;
    saveConfig();
    applyConfigToSensors();

    addLogf("MQ135 calibration result: %.3f (previous %.3f)", avgR0, oldR0);

    *pCalib = false;
    vTaskDelete(NULL);
}

// ---------------- Start Calibration ----------------
void startCalibration() {
    if (calibratingMQ135 || calibratingDust) {
        addLog("Calibration already running");
        return;
    }

    addLog("Starting calibration tasks...");

    calibratingMQ135 = true;
    calibratingDust = true;

    xTaskCreate(mq135CalibrationTask, "MQ135CalTask", 4096, &calibratingMQ135, 1, NULL);
   // xTaskCreate(dustCalibrationTask, "DustCalTask", 4096, &calibratingDust, 1, NULL);
}

// ---------------- Web Routes ----------------
void setupCalibrationRoutes() {
    server.on("/calibrate", HTTP_GET, [](AsyncWebServerRequest *request) {
        String page = R"HTML(
<html>
<head>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<title>Calibrate</title>
</head>
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

    server.on("/api/calibrate", [](AsyncWebServerRequest *request) {
        startCalibration();
        request->send(200, "text/plain", "Calibration started...");
    });
}
