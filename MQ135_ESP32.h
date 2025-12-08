#pragma once
#include <Arduino.h>
#include "MQ135.h"

class MQ135_ESP32 : public MQ135 {
public:
  MQ135_ESP32(uint8_t pin, float rzero, float rload = 10.0f)
    : MQ135(pin, rzero, rload), _pin(pin), _rload(rload) {}

  float getResistanceESP32() {
    int adc = analogRead(_pin);
    float voltage = adc * 3.3f / 4095.0f;

    addLogf("[MQ135] ADC=%d  Voltage=%.3fV", adc, voltage);

    if (adc <= 0 || adc >= 4095) {
      addLog("[MQ135] Invalid ADC range");
      return -1.0f;
    }

    float rs = ((4095.0f - adc) * _rload) / adc;
    addLogf("[MQ135] RS=%.2f kOhm", rs);
    return rs;
  }

  // Start non-blocking warmup
  void warmupStart(unsigned long duration_ms = 60000UL) {  // 1 minute
    _warmupStart = millis();
    _warmupDuration = duration_ms;
    _warmingUp = true;
    _lastWarmupLog = 0;
    addLog("[MQ135] Warm-up started");
  }

  // Non-blocking warmup loop
  void warmupLoop() {
    if (!_warmingUp) return;

    unsigned long now = millis();
    unsigned long elapsed = now - _warmupStart;

    // Log every 5 seconds only
    if (now - _lastWarmupLog >= 5000) {
      int adc = analogRead(_pin);
      float voltage = adc * 3.3f / 4095.0f;
      addLogf("[MQ135] Warm-up: ADC=%d  Voltage=%.3fV  Elapsed=%lu ms",
              adc, voltage, elapsed);
      _lastWarmupLog = now;
    }

    if (elapsed >= _warmupDuration) {
      _warmingUp = false;
      addLog("[MQ135] Warm-up complete");
    }
  }

  bool isWarmingUp() const {
    return _warmingUp;
  }

private:
  uint8_t _pin;
  float _rload;

  unsigned long _warmupStart = 0;
  unsigned long _warmupDuration = 0;
  unsigned long _lastWarmupLog = 0;
  bool _warmingUp = false;
};
