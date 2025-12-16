// File: PMS7003.cpp
// PMS7003 ESP32 Library - Full data frame support
// Based on Plantower PMS7003 Datasheet V2.5

#include "PMS7003.h"

#define PMS_START_1 0x42
#define PMS_START_2 0x4D
#define PMS_FRAME_LEN 32

PMS7003::PMS7003(HardwareSerial &serial)
    : _serial(&serial), _setPin(-1), _resetPin(-1) {}

PMS7003::PMS7003(HardwareSerial &serial, int8_t setPin, int8_t resetPin)
    : _serial(&serial), _setPin(setPin), _resetPin(resetPin) {}

void PMS7003::begin(uint32_t baud) {
    _serial->begin(baud);

    if (_setPin >= 0) {
        pinMode(_setPin, OUTPUT);
        digitalWrite(_setPin, HIGH);
    }

    if (_resetPin >= 0) {
        pinMode(_resetPin, OUTPUT);
        digitalWrite(_resetPin, HIGH);
    }
}

void PMS7003::sleep() {
    if (_setPin >= 0) {
        digitalWrite(_setPin, LOW);
    }
}

void PMS7003::wakeup() {
    if (_setPin >= 0) {
        digitalWrite(_setPin, HIGH);
        delay(30000);
    }
}

void PMS7003::reset() {
    if (_resetPin >= 0) {
        digitalWrite(_resetPin, LOW);
        delay(10);
        digitalWrite(_resetPin, HIGH);
        delay(30000);
    }
}

bool PMS7003::read(PMS7003_Data &data) {
    uint8_t buffer[PMS_FRAME_LEN];

    if (!readFrame(buffer, PMS_FRAME_LEN)) {
        return false;
    }

    uint16_t sum = 0;
    for (int i = 0; i < PMS_FRAME_LEN - 2; i++) {
        sum += buffer[i];
    }

    data.checksum = makeWord(buffer[30], buffer[31]);
    if (sum != data.checksum) {
        return false;
    }

    data.frameLength   = makeWord(buffer[2],  buffer[3]);

    data.pm1_0_cf1     = makeWord(buffer[4],  buffer[5]);
    data.pm2_5_cf1     = makeWord(buffer[6],  buffer[7]);
    data.pm10_cf1      = makeWord(buffer[8],  buffer[9]);

    data.pm1_0_atm     = makeWord(buffer[10], buffer[11]);
    data.pm2_5_atm     = makeWord(buffer[12], buffer[13]);
    data.pm10_atm      = makeWord(buffer[14], buffer[15]);

    data.particles_03um  = makeWord(buffer[16], buffer[17]);
    data.particles_05um  = makeWord(buffer[18], buffer[19]);
    data.particles_10um  = makeWord(buffer[20], buffer[21]);
    data.particles_25um  = makeWord(buffer[22], buffer[23]);
    data.particles_50um  = makeWord(buffer[24], buffer[25]);
    data.particles_100um = makeWord(buffer[26], buffer[27]);

    data.reserved      = makeWord(buffer[28], buffer[29]);

    return true;
}

bool PMS7003::readFrame(uint8_t *buffer, size_t len) {
    while (_serial->available()) {
        if (_serial->peek() == PMS_START_1) {
            _serial->read();
            if (_serial->read() == PMS_START_2) {
                buffer[0] = PMS_START_1;
                buffer[1] = PMS_START_2;
                for (size_t i = 2; i < len; i++) {
                    buffer[i] = _serial->read();
                }
                return true;
            }
        }
        _serial->read();
    }
    return false;
}

uint16_t PMS7003::makeWord(uint8_t high, uint8_t low) {
    return ((uint16_t)high << 8) | low;
}
