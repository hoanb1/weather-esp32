#include "mq135.h"
#include <math.h>

// ======================================================================
// Constants (approx based on Winsen MQ135 curves)
// ======================================================================

// Default A/B for a generalized interpolation (NH3/smoke mix)
#define GAS_A 70.0f
#define GAS_B -3.2f

// Approx temperature/humidity correction (from Fig4)
static float approxCorrection(float t, float h) {
    float kt = 1.0f + (t - 20.0f) * -0.015f;  // ~1.5% change per °C
    float kh = 1.0f + (h - 55.0f) * -0.003f; // ~0.3% change per %RH
    return kt * kh;
}

// ======================================================================
// Constructor
// ======================================================================
MQ135::MQ135(uint8_t pin, float rload, float rzero)
: _pin(pin), _rload(rload), _rzero(rzero) {}

// ======================================================================
// ADC with noise filtering
// ======================================================================
int MQ135::readADC() {
    const int N = 15;
    int buf[N];

    for (int i = 0; i < N; i++) {
        buf[i] = analogRead(_pin);
        delayMicroseconds(300);
    }

    // sort
    for (int i = 0; i < N - 1; i++) {
        for (int j = i + 1; j < N; j++) {
            if (buf[j] < buf[i]) {
                int t = buf[i];
                buf[i] = buf[j];
                buf[j] = t;
            }
        }
    }

    // average middle samples
    long sum = 0;
    for (int i = 5; i < 10; i++) sum += buf[i];

    return sum / 5;
}

// ======================================================================
// Rs calculation
// ======================================================================
float MQ135::readRs() {
    int adc = readADC();
    if (adc <= 0 || adc >= 4095) return NAN;
    return ((4095.0f / adc) - 1.0f) * _rload;
}

float MQ135::readRsCorrected(float t, float h) {
    float rs = readRs();
    if (!isfinite(rs)) return NAN;
    float cf = approxCorrection(t, h);
    return rs * cf;
}

// ======================================================================
// Generic gas index (not real ppm)
// ======================================================================
float MQ135::getIndex() {
    float rs = readRs();
    if (!isfinite(rs)) return NAN;
    float ratio = rs / _rzero;
    return GAS_A * powf(ratio, GAS_B);
}

float MQ135::getCorrectedIndex(float t, float h) {
    float rs = readRsCorrected(t, h);
    if (!isfinite(rs)) return NAN;
    float ratio = rs / _rzero;
    return GAS_A * powf(ratio, GAS_B);
}

// ======================================================================
// RZERO calculation
// ======================================================================
float MQ135::getRZero() {
    float rs = readRs();
    if (!isfinite(rs)) return NAN;
    return rs; // no fake CO2 model
}

float MQ135::getCorrectedRZero(float t, float h) {
    float rs = readRsCorrected(t, h);
    if (!isfinite(rs)) return NAN;
    return rs;
}

void MQ135::setRZero(float r0) {
    if (isfinite(r0) && r0 > 0.1f) _rzero = r0;
}

float MQ135::getStoredRZero() {
    return _rzero;
}

// ======================================================================
// Auto calibration (clean air baseline)
// ======================================================================
float MQ135::autoCalibrate(float t, float h) {
    const int N = 30;
    float sum = 0;
    int count = 0;

    for (int i = 0; i < N; i++) {
        float r0 = getCorrectedRZero(t, h);
        if (isfinite(r0)) {
            sum += r0;
            count++;
        }
        delay(10);
    }

    if (count < 30) return NAN;

    _rzero = sum / count;
    return _rzero;
}
