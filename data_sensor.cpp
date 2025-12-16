//data_sensor.cpp
#include <Arduino.h>
#include "data.h"
#include "config.h"
#include <ArduinoJson.h>
#include "mq135.h"
#include "RawPMS7003.h"

// =====================================================================
// Global instances
// =====================================================================
Adafruit_BME280 bme;
GP2YDustSensor* gp2ySensor = nullptr;
MQ135* mq135 = nullptr;
bool bmeInitialized = false;

extern float dust_baseline;

RawPMS7003* pms7003 = nullptr;
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
// Init PMS7003 Sensor
// =====================================================================
void initPMS7003() {
  if (pmsInitialized) return;

  pms7003 = new RawPMS7003(
    Serial2,
    appConfig.pmsRxPin,
    appConfig.pmsTxPin,
    appConfig.pmsSetPin);

  pms7003->begin();

  pmsInitialized = true;
  addLogf("[PMS7003] Initialized on RX:%d, TX:%d, SET:%d (9600 baud)",
          appConfig.pmsRxPin, appConfig.pmsTxPin, appConfig.pmsSetPin);
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
// Init Dust Sensor
// =====================================================================
void initDustSensor() {
  if (gp2ySensor) return;
  gp2ySensor = new GP2YDustSensor(GP2YDustSensorType::GP2Y1014AU0F,
                                  appConfig.dustLEDPin,
                                  appConfig.dustADCPin);
  gp2ySensor->begin();

  if (isfinite(appConfig.dust_baseline) && appConfig.dust_baseline > 0.0f) {
    gp2ySensor->setBaseline(appConfig.dust_baseline);

    addLogf("[gp2ySensor] Using saved baseline=%.4f", appConfig.dust_baseline);
  } else {
    addLog("[gp2ySensor] No saved baseline, will calibrate if needed");
  }

  if (isfinite(appConfig.dust_calibration) && appConfig.dust_calibration > 0.0f) {
    gp2ySensor->setCalibrationFactor(appConfig.dust_calibration);
    addLogf("[gp2ySensor] Using saved CalibrationFactor=%4f", appConfig.dust_calibration);
  } else {
    addLog("[gp2ySensor] No saved CalibrationFactor, will calibrate if needed");
  }
}

// =====================================================================
// JSON generator for MQTT
// =====================================================================
String getDataJson() {
  // --- 1. Read BME280 ---
  float t = NAN, h = NAN, p = NAN;

  if (bmeInitialized) {
    t = safeRound(bme.readTemperature(), 1);
    h = safeRound(bme.readHumidity(), 1);
    p = safeRound(bme.readPressure() / 100.0f, 1);
  }

  // --- 2. Read PMS7003 ---
  uint16_t pms_pm25_read = 0;
  int pms_aqi = -1;
  String pms_log_status = "N/A";
  bool pms_read_ok = false;

  if (pmsInitialized && pms7003) {
    if (pms7003->read()) {
      pms_read_ok = pms7003->is_valid;
      if (pms_read_ok) {
        pms_pm25_read = pms7003->data.pm2_5_std;
        pms_aqi = calcAQI_PM25((float)pms_pm25_read);
        pms_log_status = String("OK, PM2.5 Std=") + String(pms_pm25_read);
      } else {
        pms_log_status = String("Protocol Error:") + String(pms7003->status_code);
      }
    } else {
      pms_log_status = String("Read Failed, Status:") + String(pms7003->status_code);
    }
  }

  // --- 3. Read GP2Y ---
  uint16_t gp2y_pm = 0;
  int gp2y_aqi = -1;

  if (gp2ySensor) {
    gp2y_pm = gp2ySensor->getDustDensity();
    gp2y_aqi = calcAQI_PM25(gp2y_pm);
  }

  // --- Determine PM and AQI for the simplified JSON ---
  uint16_t final_pm = pms_read_ok ? pms_pm25_read : gp2y_pm;
  int final_aqi = pms_read_ok ? pms_aqi : gp2y_aqi;

  // --- 4. Read MQ135 ---
  float mqIndex = NAN;
  if (mq135 && isfinite(t) && isfinite(h)) {
    mqIndex = mq135->getCorrectedIndex(t, h);
    mqIndex = safeRound(mqIndex, 0);
  }

  uint64_t ts = nowMicros();

  // =================================================================
  // JSON Output
  // =================================================================
  StaticJsonDocument<512> doc;

  doc["id"] = appConfig.deviceId;
  if (isfinite(t)) doc["t"] = t;
  if (isfinite(h)) doc["h"] = h;
  if (isfinite(p)) doc["p"] = p;

  doc["pm"] = final_pm;
  if (final_aqi != -1) doc["aqi"] = final_aqi;
  if (isfinite(mqIndex)) doc["mq"] = mqIndex;

  doc["ts"] = ts;

  String json;
  serializeJson(doc, json);



#define PMS_LOG_BUFFER_SIZE 1024
  char pms_detail_buffer[PMS_LOG_BUFFER_SIZE] = "";

  if (pms_read_ok) {

    snprintf(
      pms_detail_buffer,
      PMS_LOG_BUFFER_SIZE,
      " | "
      "PMS7003_MASS{PM1.0_Std=%u, PM2.5_Std=%u, PM10_Std=%u (USED_AQI)} | "
      "PMS7003_MASS_ATM{PM1.0_Atm=%u, PM2.5_Atm=%u, PM10_Atm=%u (NOT_USED_AQI)} | "
      "PMS7003_PARTICLE_COUNT{PC_0.3um=%u, PC_2.5um=%u, PC_10um=%u}",
      pms7003->data.pm1_0_std, pms7003->data.pm2_5_std, pms7003->data.pm10_0_std,
      pms7003->data.pm1_0_atm, pms7003->data.pm2_5_atm, pms7003->data.pm10_0_atm,
      pms7003->data.count_0_3um, pms7003->data.count_2_5um, pms7003->data.count_10_0um);
  } else {

    pms_detail_buffer[0] = '\0';
  }


  addLogf(
    "[DATA] "
    "BME280{T=%.1f C, H=%.1f %%, P=%.1f hPa} | "
    "PMS7003{PM2.5_Std= %u ug/m3, Status=%s} | "
    "GP2Y{PM2.5_Eq=%u ug/m3, AQI_GP2Y=%d} | "
    "MQ135{AQI_MQ135=%.0f} | "
    "FinalSource=%s"
    "%s",
    t, h, p,
    pms_pm25_read, pms_log_status.c_str(),
    gp2y_pm, gp2y_aqi,
    mqIndex,
    pms_read_ok ? "PMS7003_SENSOR" : "GP2Y_SENSOR",
    pms_detail_buffer);

  return json;
}