//RawPMS7003.h
#pragma once

#include <HardwareSerial.h>
#include <Arduino.h>

class RawPMS7003 {
public:
  // Constants for the 32-byte packet structure
  static const uint8_t FRAME_LENGTH = 32;
  static const uint8_t FRAME_HEADER_1 = 0x42;
  static const uint8_t FRAME_HEADER_2 = 0x4d;

  // Data structure to hold the full 13 uint16_t fields from the 32-byte packet
  struct Measurements {
    uint16_t frameHeader;   // Index 0-1 (0x424D)
    uint16_t frameLen;      // Index 2-3 (0x001C = 28)
    uint16_t pm1_0_std;     // Index 4-5
    uint16_t pm2_5_std;     // Index 6-7
    uint16_t pm10_0_std;    // Index 8-9
    uint16_t pm1_0_atm;     // Index 10-11
    uint16_t pm2_5_atm;     // Index 12-13
    uint16_t pm10_0_atm;    // Index 14-15
    uint16_t count_0_3um;   // Index 16-17 (Number Concentration)
    uint16_t count_0_5um;   // Index 18-19
    uint16_t count_1_0um;   // Index 20-21
    uint16_t count_2_5um;   // Index 22-23
    uint16_t count_5_0um;   // Index 24-25
    uint16_t count_10_0um;  // Index 26-27
    uint16_t reserved;      // Index 28-29
    uint16_t checkSum;      // Index 30-31 (Checksum, calculated by us)
  };

  // Constructor: Takes the HardwareSerial port, RX pin, and TX pin
  RawPMS7003(HardwareSerial& serial, int rxPin, int txPin);

  // Initialization (sets baud rate and pins)
  void begin();

  // Main function to read data from the sensor
  bool read();

  // Data fields
  Measurements data;
  bool is_valid = false;
  uint8_t status_code = 0;  // 0: OK, 1: Header error, 2: Length error, 3: Checksum error, 4: Timeout

private:
  HardwareSerial* _serial;
  int _rxPin;
  int _txPin;

  // Utility function to calculate checksum
  uint16_t calculateChecksum(const uint8_t* buffer, size_t length);
};