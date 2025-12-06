//data_sensor.cpp
#include <Arduino.h> 
#include "data.h"
#include "config.h"
#include <ArduinoJson.h>
#include "time.h"
#include <Arduino.h> 
// --- Global Variable Definitions ---
Adafruit_BME280 bme;
GP2YDustSensor *dustSensor = nullptr; 
bool bmeInitialized = false;

// Fixed Constants (ESP32 ADC)
const float VCC_VOLTS = 3.3f; // ESP32 ADC reference voltage
const int ADC_MAX = 4095;    // 12-bit ADC Resolution

// External Variables
extern AsyncWebSocket ws; 
extern String latestJson; 

// -------------------------------------------------------------------
// --- Sensor Initialization ---
// -------------------------------------------------------------------

void initDustSensor() {
    if (dustSensor == NULL) {
        dustSensor = new GP2YDustSensor(
            GP2YDustSensorType::GP2Y1014AU0F, 
            appConfig.dustLEDPin, 
            appConfig.dustADCPin
        );
        dustSensor->begin();
    }
}

// -------------------------------------------------------------------
// --- System & Utility Functions ---
// -------------------------------------------------------------------

uint64_t getCurrentTimeMicroseconds() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (uint64_t)tv.tv_sec * 1000000ULL + tv.tv_usec;
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("WS: Client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        client->text(latestJson);
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("WS: Client #%u disconnected\n", client->id());
    } else if (type == WS_EVT_DATA) {
        // ignore for now
    }
}

void notifyClients(String json) {
    ws.textAll(json);
}

// -------------------------------------------------------------------
// --- MQ Sensor Calculation Helpers ---
// -------------------------------------------------------------------

/**
 * Convert ADC raw reading (0..ADC_MAX) -> sensor output voltage (V)
 */
static float adcToVoltage(int adc_raw) {
    // Protect against invalid adc_raw
    if (adc_raw <= 0) return 0.0f;
    if (adc_raw > ADC_MAX) adc_raw = ADC_MAX;
    return (float)adc_raw * VCC_VOLTS / (float)ADC_MAX;
}

/**
 * Calculate sensor resistance Rs (kOhm) from ADC raw.
 * Formula derived from voltage divider: Rs = (Vcc - Vout) * RL / Vout
 * RL (kOhm) is appConfig.mq_rl_kohm
 */
static float calculateRs_kOhm(int mq_raw) {
    float vout = adcToVoltage(mq_raw);
    if (vout <= 0.0001f) return INFINITY; // avoid divide by zero
    float rl = appConfig.mq_rl_kohm; // kOhm
    // Rs in kOhm
    float rs = ((VCC_VOLTS - vout) * rl) / vout;
    return rs;
}

/**
 * Estimate sensor R0 (kOhm) using configured baseline ratio.
 * NOTE: This is a rough estimate. Ideal workflow: perform calibration once in known-clean-air,
 * call a calibration routine to store exact R0 into non-volatile storage.
 *
 * If mq_r0_ratio_clean is the expected Rs/R0 in clean air, then:
 *    R0_est = Rs_clean / (Rs/R0) = Rs_clean / mq_r0_ratio_clean
 *
 * Here we estimate R0 from current Rs measurement using baseline ratio.
 */
static float estimateR0_kOhm_fromBaselineRs(float rs_kohm) {
    if (!isfinite(rs_kohm) || rs_kohm <= 0.0f) return NAN;
    float ratio = appConfig.mq_r0_ratio_clean;
    if (ratio <= 0.0f) return NAN;
    return rs_kohm / ratio;
}

/**
 * Compute Rs/R0 ratio.
 * If you have a stored/calibrated R0, prefer to use that. This function uses the
 * rough estimate method when exact R0 not available.
 */
static float calculateRsOverR0(int mq_raw) {
    float rs = calculateRs_kOhm(mq_raw);
    if (!isfinite(rs)) return NAN;
    float r0_est = estimateR0_kOhm_fromBaselineRs(rs);
    if (!isfinite(r0_est) || r0_est <= 0.0f) return NAN;
    return rs / r0_est;
}

// -------------------------------------------------------------------
// --- TVOC estimation ---
// -------------------------------------------------------------------
/**
 * Calculates estimated TVOC concentration in PPM from the Rs/R0 ratio using a power-law curve.
 *
 * NOTE:
 *  - MQ-series sensors are not selective: TVOC estimate depends on curve constants and calibration.
 *  - There is NO universally accepted "TVOC ppm" mapping for MQ-135 without calibration.
 *  - The curve constants (tvoc_a_curve, tvoc_b_curve) are sensor-specific and should be tuned.
 *
 * We **do not** clamp to 400 anymore (that caused constant 400 reading).
 */
float calculateTVOC_PPM(int mq_raw) {
    float rs_ro = calculateRsOverR0(mq_raw);
    if (!isfinite(rs_ro) || rs_ro <= 0.0f) {
        // unable to compute reliable ratio: return NAN to indicate invalid measurement
        return NAN;
    }

    // Apply power law curve: PPM = A * (Rs/R0)^B
    // (A and B stored in appConfig, legacy values kept for backward compatibility)
    float A = appConfig.tvoc_a_curve;
    float B = appConfig.tvoc_b_curve;

    // Protect against crazy values
    if (!isfinite(A) || !isfinite(B)) return NAN;

    float ppm = A * powf(rs_ro, B);

    // Clamp to reasonable bounds (0 .. 10000 ppm). You can adjust upper bound if needed.
    if (!isfinite(ppm)) return NAN;
    if (ppm < 0.0f) ppm = 0.0f;
    if (ppm > 10000.0f) ppm = 10000.0f;

    return ppm;
}

// -------------------------------------------------------------------
// --- AQI (PM2.5) using EPA breakpoints and linear interpolation ---
// -------------------------------------------------------------------

/**
 * Linear interpolate helper for AQI between breakpoints.
 */
static int linearAQI(float Cp, float Clow, float Chigh, int Ilow, int Ihigh) {
    if (Chigh == Clow) return Ilow;
    float a = (Ihigh - Ilow) / (Chigh - Clow);
    float val = a * (Cp - Clow) + Ilow;
    return (int)roundf(val);
}

/**
 * Calculate AQI for PM2.5 concentration (µg/m3) using EPA breakpoints.
 * Uses the standard piecewise linear interpolation.
 */
int calcAQI_PM25(float pm25) {
    // pm25 expected in µg/m3
    if (pm25 < 0.0f || !isfinite(pm25)) return -1;

    // Breakpoints from EPA (PM2.5 - µg/m3)
    // 0.0 - 12.0   -> 0 - 50
    // 12.1 - 35.4  -> 51 - 100
    // 35.5 - 55.4  -> 101 - 150
    // 55.5 - 150.4 -> 151 - 200
    // 150.5 - 250.4-> 201 - 300
    // 250.5 - 350.4-> 301 - 400
    // 350.5 - 500.4-> 401 - 500

    if (pm25 <= 12.0f) {
        return linearAQI(pm25, 0.0f, 12.0f, 0, 50);
    } else if (pm25 <= 35.4f) {
        return linearAQI(pm25, 12.1f, 35.4f, 51, 100);
    } else if (pm25 <= 55.4f) {
        return linearAQI(pm25, 35.5f, 55.4f, 101, 150);
    } else if (pm25 <= 150.4f) {
        return linearAQI(pm25, 55.5f, 150.4f, 151, 200);
    } else if (pm25 <= 250.4f) {
        return linearAQI(pm25, 150.5f, 250.4f, 201, 300);
    } else if (pm25 <= 350.4f) {
        return linearAQI(pm25, 250.5f, 350.4f, 301, 400);
    } else if (pm25 <= 500.4f) {
        return linearAQI(pm25, 350.5f, 500.4f, 401, 500);
    } else {
        return 500; // beyond scale
    }
}

// -------------------------------------------------------------------
// --- IAQI / AQI mapping for TVOC (Custom mapping) ---
// -------------------------------------------------------------------
/*
 * NOTE:
 *  - There is no single internationally-adopted AQI for TVOC ppm produced by cheap sensors.
 *  - The mapping below is a pragmatic/internal mapping to produce a "TVOC-based IAQI".
 *  - Adjust breakpoints based on your environment and calibrated measurements.
 *
 * Suggested mapping (tvoc_ppm):
 *  0.000 - 0.300 ppm  => Good (0 - 50)
 *  0.301 - 0.600 ppm  => Moderate (51 - 100)
 *  0.601 - 1.000 ppm  => Unhealthy for sensitive groups (101 - 150)
 *  1.001 - 3.000 ppm  => Unhealthy / Very Unhealthy (151 - 200+)
 *  > 3.000 ppm        => Hazardous (201+)
 *
 * The numeric ranges and scaling are *user-configurable* — tune them after calibration.
 */

int calcAQI_TVOC(float tvoc_ppm) {
    if (!isfinite(tvoc_ppm) || tvoc_ppm < 0.0f) return -1;

    // Use ppm as provided by calculateTVOC_PPM (note: that function returns ppm)
    if (tvoc_ppm <= 0.300f) {
        return linearAQI(tvoc_ppm, 0.0f, 0.300f, 0, 50);
    } else if (tvoc_ppm <= 0.600f) {
        return linearAQI(tvoc_ppm, 0.301f, 0.600f, 51, 100);
    } else if (tvoc_ppm <= 1.000f) {
        return linearAQI(tvoc_ppm, 0.601f, 1.000f, 101, 150);
    } else if (tvoc_ppm <= 3.000f) {
        return linearAQI(tvoc_ppm, 1.001f, 3.000f, 151, 200);
    } else {
        // Above 3 ppm -> map into 201..500 range (rough)
        // We'll scale proportionally up to 10 ppm to reach 500, beyond that clamp.
        float Clow = 3.001f, Chigh = 10.0f;
        int Ilow = 201, Ihigh = 500;
        float Cp = tvoc_ppm;
        if (Cp >= Chigh) return 500;
        return linearAQI(Cp, Clow, Chigh, Ilow, Ihigh);
    }
}

// -------------------------------------------------------------------
// --- Main Data Getter ---
// -------------------------------------------------------------------

String getDataJson() {
    // Read BME280
    float t = NAN, h = NAN, p = NAN;
   if (bmeInitialized) {
        t = bme.readTemperature();
        h = bme.readHumidity();
        p = bme.readPressure() / 100.0F; // Pa -> hPa
    }

    // Read MQ Sensor
    int mq_raw = analogRead(appConfig.mqADCPin);
    float mq_tvoc_ppm = calculateTVOC_PPM(mq_raw); // ppm (may be NAN if failed)

    // Read Dust Sensor & Calculate AQI
    uint16_t dust = 0;
    if (dustSensor != nullptr) {
        dust = dustSensor->getDustDensity();
    }
    // Interpret dust reading as PM2.5 ug/m3 if your dustSensor returns that (ensure unit consistency)
    float pm25 = (float)dust;
    int aqi_pm25 = calcAQI_PM25(pm25);

    int aqi_tvoc = -1;
    if (isfinite(mq_tvoc_ppm)) {
        aqi_tvoc = calcAQI_TVOC(mq_tvoc_ppm);
    }

    // Determine Combined AQI (Max of the two valid indices)
    int combined_aqi = aqi_pm25;
    if (aqi_tvoc > combined_aqi) combined_aqi = aqi_tvoc;

    int wifiRssi = WiFi.RSSI();
    uint64_t createdAt = getCurrentTimeMicroseconds();

    Serial.printf("[DATA] T=%.1f H=%.1f P=%.1f Dust=%u MQ_Raw=%d TVOC_PPM=%s Rs/R0=%.3f AQI=%d WiFi=%d Time=%llu\n",
                  isnan(t) ? 0.0f : t,
                  isnan(h) ? 0.0f : h,
                  isnan(p) ? 0.0f : p,
                  dust,
                  mq_raw,
                  (isfinite(mq_tvoc_ppm) ? String(mq_tvoc_ppm,1).c_str() : "nan"),
                  calculateRsOverR0(mq_raw),
                  combined_aqi,
                  wifiRssi,
                  createdAt);

    // Create JSON document 
    StaticJsonDocument<512> doc;
    if (isfinite(t)) doc["temp"] = t;
    if (isfinite(h)) doc["humid"] = h;
    if (isfinite(p)) doc["pressure"] = p;
    doc["dust"] = dust;
    doc["mq135_raw"] = mq_raw; 
    doc["mq_rs_ro"] = calculateRsOverR0(mq_raw); 
    if (isfinite(mq_tvoc_ppm)) doc["mq_tvoc_ppm"] = mq_tvoc_ppm; 
    else doc["mq_tvoc_ppm"] = nullptr;
    doc["aqi"] = combined_aqi;
    doc["aqi_pm25"] = aqi_pm25;
    doc["aqi_tvoc"] = aqi_tvoc;
    doc["wifiRssi"] = wifiRssi;
    doc["createdAt"] = createdAt;

    String json;
    serializeJson(doc, json);
    return json;
}