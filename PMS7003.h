// File: PMS7003.h
// PMS7003 ESP32 Library - Full data frame support
// Based on Plantower PMS7003 Datasheet V2.5

#ifndef PMS7003_H
#define PMS7003_H

#include <Arduino.h>

struct PMS7003_Data {
    uint16_t frameLength;

    uint16_t pm1_0_cf1;
    uint16_t pm2_5_cf1;
    uint16_t pm10_cf1;

    uint16_t pm1_0_atm;
    uint16_t pm2_5_atm;
    uint16_t pm10_atm;

    uint16_t particles_03um;
    uint16_t particles_05um;
    uint16_t particles_10um;
    uint16_t particles_25um;
    uint16_t particles_50um;
    uint16_t particles_100um;

    uint16_t reserved;

    uint16_t checksum;
};

class PMS7003 {
public:
    PMS7003(HardwareSerial &serial);
    PMS7003(HardwareSerial &serial, int8_t setPin, int8_t resetPin);

    void begin(uint32_t baud = 9600);

    bool read(PMS7003_Data &data);

    void sleep();
    void wakeup();
    void reset();

private:
    HardwareSerial *_serial;
    int8_t _setPin;
    int8_t _resetPin;

    bool readFrame(uint8_t *buffer, size_t len);
    uint16_t makeWord(uint8_t high, uint8_t low);
};

#endif
