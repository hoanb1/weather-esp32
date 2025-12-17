// File: data_sensor.cpp
// Comprehensive AQI evaluation using available sensors
// JSON structure preserved

#include <Arduino.h>
#include "data.h"
#include "config.h"
#include <ArduinoJson.h>
#include "mq135.h"
#include "PMS7003.h"

// =====================================================================
// Global instances
// =====================================================================
Adafruit_BME280 bme;
GP2YDustSensor* gp2ySensor = nullptr;
MQ135* mq135 = nullptr;
bool bmeInitialized = false;

extern float dust_baseline;

PMS7003* pms7003 = nullptr;
bool pmsInitialized = false;

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
// AQI helpers
// =====================================================================
static int linAQI(float Cp, float Cl, float Ch, int Il, int Ih) {
  return (int)roundf((Ih - Il) / (Ch - Cl) * (Cp - Cl) + Il);
}

// PM2.5 AQI (EPA)
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

// PM10 AQI (EPA)
int calcAQI_PM10(float pm10) {
  if (pm10 < 0 || !isfinite(pm10)) return -1;
  if (pm10 <= 54) return linAQI(pm10, 0, 54, 0, 50);
  if (pm10 <= 154) return linAQI(pm10, 55, 154, 51, 100);
  if (pm10 <= 254) return linAQI(pm10, 155, 254, 101, 150);
  if (pm10 <= 354) return linAQI(pm10, 255, 354, 151, 200);
  if (pm10 <= 424) return linAQI(pm10, 355, 424, 201, 300);
  return linAQI(pm10, 425, 604, 301, 500);
}

// MQ135 qualitative VOC impact (heuristic)
int calcAQI_VOC(float idx) {
  if (!isfinite(idx)) return -1;
  if (idx < 100) return 25;
  if (idx < 200) return 75;
  if (idx < 300) return 125;
  if (idx < 400) return 175;
  return 250;
}

// =====================================================================
// Init PMS7003 Sensor
// =====================================================================
void initPMS7003() {
  if (pmsInitialized) return;

  Serial2.begin(9600, SERIAL_8N1,
                appConfig.pmsRxPin,
                appConfig.pmsTxPin);

  if (appConfig.pmsSetPin >= 0) {
    pms7003 = new PMS7003(Serial2, appConfig.pmsSetPin, -1);
  } else {
    pms7003 = new PMS7003(Serial2);
  }

  pms7003->begin();
  pmsInitialized = true;

  addLogf(
    "[PMS7003] Initialized RX=%d TX=%d SET=%d",
    appConfig.pmsRxPin,
    appConfig.pmsTxPin,
    appConfig.pmsSetPin);
}

// =====================================================================
// Init MQ135
// =====================================================================
void initMQ135() {
  if (mq135) return;

  float r0 = appConfig.mq_rzero;

  if (!isfinite(r0) || r0 <= 0.0f || r0 > 10000.0f) {
    addLog("[MQ135] Invalid RZERO, using default (80.0)");
    r0 = 80.0f;
  } else {
    addLogf("[MQ135] Loaded saved RZERO=%.3f", r0);
  }

  mq135 = new MQ135(appConfig.mqADCPin,
                    appConfig.mq_rl_kohm,
                    r0);
}

// =====================================================================
// Init GP2Y Dust Sensor
// =====================================================================
void initDustSensor() {
  if (gp2ySensor) return;
  gp2ySensor = new GP2YDustSensor(GP2YDustSensorType::GP2Y1014AU0F,
                                  appConfig.dustLEDPin,
                                  appConfig.dustADCPin);
  gp2ySensor->begin();

  addLogf(
    "[INIT] GP2Y LED=%d ADC=%d",
    appConfig.dustLEDPin,
    appConfig.dustADCPin);

  if (isfinite(appConfig.dust_baseline) && appConfig.dust_baseline > 0.0f) {
    gp2ySensor->setBaseline(appConfig.dust_baseline);
    addLogf("[GP2Y] Using baseline=%.4f", appConfig.dust_baseline);
  }

  if (isfinite(appConfig.dust_calibration) && appConfig.dust_calibration > 0.0f) {
    gp2ySensor->setCalibrationFactor(appConfig.dust_calibration);
    addLogf("[GP2Y] Using calibration=%.4f", appConfig.dust_calibration);
  }
}

// =====================================================================
// JSON generator for MQTT
// =====================================================================
String getDataJson() {
  // -------------------------------------------------------------------
  // 1. BME280
  // -------------------------------------------------------------------
  float t = NAN, h = NAN, p = NAN;

  if (bmeInitialized) {
    t = safeRound(bme.readTemperature(), 1);
    h = safeRound(bme.readHumidity(), 1);
    p = safeRound(bme.readPressure() / 100.0f, 1);
  }

  // -------------------------------------------------------------------
  // 2. PMS7003
  // -------------------------------------------------------------------
  PMS7003_Data pmsData;
  bool pms_ok = false;
  uint16_t pms_pm25 = 0;
  uint16_t pms_pm10 = 0;

  if (pmsInitialized && pms7003 && pms7003->read(pmsData)) {
    pms_ok = true;
    pms_pm25 = pmsData.pm2_5_atm;
    pms_pm10 = pmsData.pm10_atm;
  }

  // -------------------------------------------------------------------
  // 3. GP2Y
  // -------------------------------------------------------------------
  uint16_t gp2y_pm = gp2ySensor ? gp2ySensor->getDustDensity() : 0;

  // -------------------------------------------------------------------
  // 4. MQ135
  // -------------------------------------------------------------------
  float mqIndex = NAN;
  if (mq135 && isfinite(t) && isfinite(h)) {
    mqIndex = safeRound(mq135->getCorrectedIndex(t, h), 0);
  }

  // -------------------------------------------------------------------
  // AQI comprehensive evaluation
  // -------------------------------------------------------------------
  int aqi_pm25_pms = pms_ok ? calcAQI_PM25(pms_pm25) : -1;
  int aqi_pm25_gp2y = gp2ySensor ? calcAQI_PM25(gp2y_pm) : -1;
  int aqi_pm10 = pms_ok ? calcAQI_PM10(pms_pm10) : -1;
  int aqi_voc = isfinite(mqIndex) ? calcAQI_VOC(mqIndex) : -1;


  int final_aqi = -1;

  if (aqi_pm25_pms >= 0) {
    final_aqi = aqi_pm25_pms;
  } else if (aqi_pm25_gp2y >= 0) {
    final_aqi = aqi_pm25_gp2y;
  } else if (aqi_pm10 >= 0) {
    final_aqi = aqi_pm10;
  } else if (aqi_voc >= 0) {
    final_aqi = aqi_voc;
  }

  uint16_t final_pm = pms_ok ? pms_pm25 : gp2y_pm;
  uint64_t ts = nowMicros();

  // =================================================================
  // JSON Output (structure preserved)
  // =================================================================
  StaticJsonDocument<512> doc;

  doc["id"] = appConfig.deviceId;
  if (isfinite(t)) doc["t"] = t;
  if (isfinite(h)) doc["h"] = h;
  if (isfinite(p)) doc["p"] = p;

  if (pms_ok) {
    doc["pm25_pms"] = pms_pm25;
    doc["pm10_pms"] = pms_pm10;
    if (aqi_pm25_pms >= 0) doc["aqi_pms25"] = aqi_pm25_pms;
    if (aqi_pm10 >= 0) doc["aqi_pms10"] = aqi_pm10;
  }

  if (gp2ySensor) {
    doc["pm25_gp2y"] = gp2y_pm;
    if (aqi_pm25_gp2y >= 0) doc["aqi_gp2y25"] = aqi_pm25_gp2y;
  }

  if (isfinite(mqIndex)) {
    doc["mq"] = mqIndex;
    if (aqi_voc >= 0) doc["aqi_voc"] = aqi_voc;
  }

  doc["pm"] = final_pm;
  if (final_aqi >= 0) doc["aqi"] = final_aqi;
  doc["ts"] = ts;

  String json;
  serializeJson(doc, json);

  // =================================================================
  // Detailed log
  // =================================================================
  addLogf(
    "[DATA] BME{T=%.1fC H=%.1f%%} | "
    "PMS{PM2.5=%u PM10=%u} | "
    "Particles{>0.3um:%u, >0.5um:%u, >1.0um:%u, >2.5um:%u, >5.0um:%u, >10um:%u} | "
    "AQI=%d SRC=%s",
    t, h,
    pms_pm25, pms_pm10,
    pmsData.particles_03um, pmsData.particles_05um, pmsData.particles_10um,
    pmsData.particles_25um, pmsData.particles_50um, pmsData.particles_100um,
    final_aqi, pms_ok ? "PMS7003" : "GP2Y");

  return json;
}