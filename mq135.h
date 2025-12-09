#pragma once
#include <Arduino.h>

class MQ135 {
public:
    MQ135(uint8_t pin, float rload = 10.0f, float rzero = 80.0f);

    float readRs();
    float readRsCorrected(float t, float h);

    float getIndex();
    float getCorrectedIndex(float t, float h);

    float getRZero();
    float getCorrectedRZero(float t, float h);
    void setRZero(float r0);
    float getStoredRZero();

    float autoCalibrate(float temp, float hum);

private:
    uint8_t _pin;
    float _rload;
    float _rzero;

    int readADC();
};
