#include <Arduino.h>
#include "data.h"
#include "config.h"
#include <ArduinoJson.h>
#include "MQ135_ESP32.h"

// -------------------------------------------------------------------
// Global Sensor Instances
// -------------------------------------------------------------------
Adafruit_BME280 bme;
GP2YDustSensor* dustSensor = nullptr;
bool bmeInitialized = false;

// MQ135 ESP32 instance
MQ135_ESP32* mq135 = nullptr;

// Calibration state
bool mqCalibrating = false;
int mqCalibSamples = 0;
float mqCalibTotal = 0.0f;
unsigned long mqLastCalibMillis = 0;

const unsigned long MQ_CALIB_INTERVAL = 200;   // ms
const int MQ_CALIB_MAX_SAMPLES = 50;


// -------------------------------------------------------------------
// MQ135 Initialization
// -------------------------------------------------------------------
void initMQ135() {
    if (mq135) {
        addLog("[MQ135] Already initialized");
        return;
    }

    addLogf("[MQ135] Initializing on ADC pin %d", appConfig.mqADCPin);

    mq135 = new MQ135_ESP32(appConfig.mqADCPin, appConfig.mq_rzero, 10.0f);

    delay(100);

    int adc = analogRead(appConfig.mqADCPin);
    float volt = adc * 3.3f / 4095.0f;

    addLogf("[MQ135] Initial ADC=%d  Voltage=%.3fV", adc, volt);

    if (adc == 0) {
        addLog("[MQ135][Warning] ADC=0. Check wiring or sensor connection.");
    }

    mq135->warmupStart();
    addLog("[MQ135] Warm-up started");

    if (appConfig.mq_rzero <= 0.0f) {
        addLog("[MQ135] No stored RZero. Running calibration.");
        startMQ135Calibration();
    } else {
        addLogf("[MQ135] Using stored RZero=%.2f", appConfig.mq_rzero);
    }
}


// -------------------------------------------------------------------
// Start MQ135 Calibration
// -------------------------------------------------------------------
void startMQ135Calibration() {
    if (appConfig.mq_rzero <= 0.0f) {
        addLog("[MQ135] Starting calibration. Keep sensor in clean air.");
        mqCalibrating = true;
        mqCalibSamples = 0;
        mqCalibTotal = 0.0f;
        mqLastCalibMillis = millis();
    }
}


// -------------------------------------------------------------------
// Process MQ135 Calibration (call periodically)
// -------------------------------------------------------------------
void processMQ135Calibration(MQ135_ESP32& sensor) {
    if (!mqCalibrating) return;

    unsigned long now = millis();
    if (now - mqLastCalibMillis < MQ_CALIB_INTERVAL) return;
    mqLastCalibMillis = now;

    float rs = sensor.getResistanceESP32();
    if (!isfinite(rs) || rs <= 0.0f) {
        addLog("[MQ135] Invalid Rs reading, skipping sample");
        return;
    }

    if (appConfig.mq_r0_ratio_clean <= 0.0f) {
        appConfig.mq_r0_ratio_clean = 3.6f;
    }

    float currentRZero = rs / appConfig.mq_r0_ratio_clean;
    mqCalibTotal += currentRZero;
    mqCalibSamples++;

    addLogf("[MQ135] Calibration sample %d: RZero=%.2f",
            mqCalibSamples, currentRZero);

    if (mqCalibSamples >= MQ_CALIB_MAX_SAMPLES) {
        float rzero = mqCalibTotal / mqCalibSamples;
        appConfig.mq_rzero = rzero;
        saveConfig();

        addLogf("[MQ135] Calibration complete. Saved RZero=%.2f", rzero);
        mqCalibrating = false;
    }
}


// -------------------------------------------------------------------
// Dust Sensor Initialization
// -------------------------------------------------------------------
void initDustSensor() {
    if (!dustSensor) {
        dustSensor = new GP2YDustSensor(
            GP2YDustSensorType::GP2Y1014AU0F,
            appConfig.dustLEDPin,
            appConfig.dustADCPin
        );
        dustSensor->begin();
    }
}


// -------------------------------------------------------------------
// Utility
// -------------------------------------------------------------------
uint64_t getCurrentTimeMicroseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + tv.tv_usec;
}


// -------------------------------------------------------------------
// WebSocket Events
// -------------------------------------------------------------------
void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len) 
{
    if (type == WS_EVT_CONNECT) {
        addLogf("WS: Client #%u connected from %s",
                client->id(), client->remoteIP().toString().c_str());
        client->text(latestJson);
    } 
    else if (type == WS_EVT_DISCONNECT) {
        addLogf("WS: Client #%u disconnected", client->id());
    }
}

void notifyClients(String json) {
    ws.textAll(json);
}


// -------------------------------------------------------------------
// AQI Calculation
// -------------------------------------------------------------------
static int linearAQI(float Cp, float Clow, float Chigh, int Ilow, int Ihigh) {
    if (Chigh == Clow) return Ilow;
    return (int)roundf((Ihigh - Ilow) / (Chigh - Clow) * (Cp - Clow) + Ilow);
}

int calcAQI_PM25(float pm25) {
    if (pm25 < 0.0f || !isfinite(pm25)) return -1;

    if (pm25 <= 12.0f)   return linearAQI(pm25, 0.0f, 12.0f, 0, 50);
    if (pm25 <= 35.4f)  return linearAQI(pm25, 12.1f, 35.4f, 51, 100);
    if (pm25 <= 55.4f)  return linearAQI(pm25, 35.5f, 55.4f, 101, 150);
    if (pm25 <= 150.4f) return linearAQI(pm25, 55.5f, 150.4f, 151, 200);
    if (pm25 <= 250.4f) return linearAQI(pm25, 150.5f, 250.4f, 201, 300);
    if (pm25 <= 350.4f) return linearAQI(pm25, 250.5f, 350.4f, 301, 400);
    if (pm25 <= 500.4f) return linearAQI(pm25, 350.5f, 500.4f, 401, 500);

    return 500;
}

static float safe_round(float val, int decimals) {
    if (!isfinite(val)) return NAN;
    float factor = powf(10.0f, (float)decimals);
    return roundf(val * factor) / factor;
}


// -------------------------------------------------------------------
// Main JSON Data Builder
// -------------------------------------------------------------------
String getDataJson() {
    float t = NAN, h = NAN, p = NAN;

    if (bmeInitialized) {
        t = bme.readTemperature();
        h = bme.readHumidity();
        p = bme.readPressure() / 100.0f;
    }

    int mq_raw = analogRead(appConfig.mqADCPin);
    float mq_co2_ppm = mq135->getCorrectedPPM(t, h);

    uint16_t dust = dustSensor ? dustSensor->getDustDensity() : 0;
    float pm25 = (float)dust;
    int aqi_pm25 = calcAQI_PM25(pm25);

    uint64_t ts = getCurrentTimeMicroseconds();

    t = safe_round(t, 1);
    h = safe_round(h, 1);
    mq_co2_ppm = safe_round(mq_co2_ppm, 0);

    StaticJsonDocument<256> doc;

    doc["id"] = appConfig.deviceId;

    if (isfinite(t)) doc["t"] = t;
    if (isfinite(h)) doc["h"] = h;
    if (isfinite(p)) doc["p"] = p;

    doc["pm"]  = dust;
    doc["mq"]  = mq_raw;
    doc["c"]   = mq_co2_ppm;
    doc["aqi"] = aqi_pm25;
    doc["ts"]  = ts;

    String json;
    serializeJson(doc, json);
    return json;
}
