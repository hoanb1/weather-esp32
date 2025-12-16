// File: RawPMS7003.h
#pragma once

#include <HardwareSerial.h>
#include <Arduino.h>

class RawPMS7003 {
public:
  // Constants for the 32-byte packet structure
  static const uint8_t FRAME_LENGTH = 32;
  static const uint8_t FRAME_HEADER_1 = 0x42;
  static const uint8_t FRAME_HEADER_2 = 0x4d;

  // Data structure to hold the core data fields
  struct Measurements {
    uint16_t pm1_0_std;
    uint16_t pm2_5_std;
    uint16_t pm10_0_std;
    uint16_t pm1_0_atm;
    uint16_t pm2_5_atm;
    uint16_t pm10_0_atm;
    uint16_t count_0_3um;
    uint16_t count_0_5um;
    uint16_t count_1_0um;
    uint16_t count_2_5um;
    uint16_t count_5_0um;
    uint16_t count_10_0um;
  };

  // Constructor: 'setPin' is optional, default is -1 (Always Active Mode)
  RawPMS7003(HardwareSerial& serial, int rxPin, int txPin, int setPin = -1);

  void begin();
  bool read();

  // Public methods for manual sleep/wake (only effective if setPin >= 0)
  void sleep();
  void wake();

  Measurements data;
  bool is_valid = false;
  uint8_t status_code = 0;  // 0: OK, 2: Error, 3: Checksum, 4: Timeout

private:
  HardwareSerial* _serial;
  int _rxPin;
  int _txPin;
  int _setPin;  // Pin for SET/ENA control (Pin 10)

  uint16_t calculateChecksum(const uint8_t* buffer, size_t length);
};