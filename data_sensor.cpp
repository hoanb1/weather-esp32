//data_sensor.cpp
#include <Arduino.h>
#include "data.h"
#include "config.h"
#include <ArduinoJson.h>
#include "mq135.h"

// =====================================================================
// Global instances
// =====================================================================
Adafruit_BME280 bme;
GP2YDustSensor* dustSensor = nullptr;
MQ135* mq135 = nullptr;
bool bmeInitialized = false;

// =====================================================================
// Utility
// =====================================================================
static float safeRound(float v, int dec) {
    if (!isfinite(v)) return NAN;
    float f = powf(10.0f, dec);
    return roundf(v * f) / f;
}

uint64_t nowMicros() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + tv.tv_usec;
}

// =====================================================================
// AQI PM2.5
// =====================================================================
static int linAQI(float Cp, float Cl, float Ch, int Il, int Ih) {
    return (int)roundf((Ih - Il) / (Ch - Cl) * (Cp - Cl) + Il);
}

int calcAQI_PM25(float pm) {
    if (pm < 0 || !isfinite(pm)) return -1;
    if (pm <= 12.0f) return linAQI(pm, 0.0f, 12.0f, 0, 50);
    if (pm <= 35.4f) return linAQI(pm, 12.1f, 35.4f, 51, 100);
    if (pm <= 55.4f) return linAQI(pm, 35.5f, 55.4f, 101, 150);
    if (pm <= 150.4f) return linAQI(pm, 55.5f, 150.4f, 151, 200);
    if (pm <= 250.4f) return linAQI(pm, 150.5f, 250.4f, 201, 300);
    if (pm <= 350.4f) return linAQI(pm, 250.5f, 350.4f, 301, 400);
    return linAQI(pm, 350.5f, 500.4f, 401, 500);
}

// =====================================================================
// Init MQ135
// =====================================================================
void initMQ135() {
    if (mq135) return;

    float r0 = appConfig.mq_rzero;
    if (!isfinite(r0) || r0 < 1.0f) r0 = 80.0f;

    mq135 = new MQ135(appConfig.mqADCPin,
                      appConfig.mq_rl_kohm,
                      r0);

    addLogf("[MQ135] R0=%.2f", r0);
}

// =====================================================================
// Init Dust Sensor
// =====================================================================
void initDustSensor() {
    if (dustSensor) return;
    dustSensor = new GP2YDustSensor(GP2YDustSensorType::GP2Y1014AU0F,
                                    appConfig.dustLEDPin,
                                    appConfig.dustADCPin);
    dustSensor->begin();
}

// =====================================================================
// JSON generator for MQTT (small payload)
// =====================================================================
String getDataJson() {
    float t = NAN, h = NAN, p = NAN;

    if (bmeInitialized) {
        t = safeRound(bme.readTemperature(), 1);
        h = safeRound(bme.readHumidity(), 1);
        p = safeRound(bme.readPressure() / 100.0f, 1);
    }

    uint16_t pm = dustSensor ? dustSensor->getDustDensity() : 0;
    int aqi = calcAQI_PM25(pm);

    float mqIndex = NAN;
    if (mq135 && isfinite(t) && isfinite(h)) {
        mqIndex = mq135->getCorrectedIndex(t, h);
        mqIndex = safeRound(mqIndex, 0);
    }

    uint64_t ts = nowMicros();

    StaticJsonDocument<256> doc;
    doc["id"] = appConfig.deviceId;
    if (isfinite(t)) doc["t"] = t;
    if (isfinite(h)) doc["h"] = h;
    if (isfinite(p)) doc["p"] = p;

    doc["pm"] = pm;
    doc["aqi"] = aqi;

    if (isfinite(mqIndex)) doc["mq"] = mqIndex;

    doc["ts"] = ts;

    String json;
    serializeJson(doc, json);

    addLogf("[DEBUG] T=%.1f H=%.1f P=%.1f PM=%u AQI=%d MQ=%.0f",
            t, h, p, pm, aqi, mqIndex);

    return json;
}
