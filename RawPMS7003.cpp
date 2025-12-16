// File: RawPMS7003.cpp
#include "RawPMS7003.h"

// 30 seconds wait time after wake/power on (required by datasheet)
#define PMS_FAN_STARTUP_DELAY 30000

// Constructor: Added setPin with default value -1
RawPMS7003::RawPMS7003(HardwareSerial& serial, int rxPin, int txPin, int setPin)
  : _serial(&serial), _rxPin(rxPin), _txPin(txPin), _setPin(setPin) {
}

// Initialization
void RawPMS7003::begin() {
  // Only configure the pin if it's explicitly set (>= 0)
  if (_setPin >= 0) {
    pinMode(_setPin, OUTPUT);
    sleep();  // Start in sleep mode if controlled
  }

  _serial->begin(9600, SERIAL_8N1, _rxPin, _txPin);

  // If in Active Mode (setPin < 0), flush serial buffer briefly
  if (_setPin < 0) {
    delay(100);
    while (_serial->available()) _serial->read();
  }
}

// Set pin to LOW for Sleep Mode
void RawPMS7003::sleep() {
  if (_setPin >= 0) {
    digitalWrite(_setPin, LOW);
  }
}

// Set pin to HIGH for Working Mode
void RawPMS7003::wake() {
  if (_setPin >= 0) {
    digitalWrite(_setPin, HIGH);
  }
}

// Checksum calculation (Sum of all bytes up to the checksum field)
uint16_t RawPMS7003::calculateChecksum(const uint8_t* buffer, size_t length) {
  uint16_t sum = 0;
  // Sum all bytes *excluding* the last two bytes (which contain the checksum itself)
  for (size_t i = 0; i < length - 2; ++i) {
    sum += buffer[i];
  }
  return sum;
}

// Main read function
bool RawPMS7003::read() {
  is_valid = false;
  status_code = 0;

  // --- 0. Control SET/ENA pin if configured ---
  if (_setPin >= 0) {
    wake();
    // MUST wait for the fan to stabilize (30 seconds)
    delay(PMS_FAN_STARTUP_DELAY);
  }

  // Flush any old data in the buffer after wake/startup
  while (_serial->available()) _serial->read();

  // --- 1. Find Header (0x42, 0x4D) ---
  uint8_t buffer[FRAME_LENGTH];
  unsigned long timeout = millis() + 1000;

  while (millis() < timeout) {
    if (_serial->available() >= 2) {
      uint8_t byte1 = _serial->read();
      uint8_t byte2 = _serial->read();

      if (byte1 == FRAME_HEADER_1 && byte2 == FRAME_HEADER_2) {
        buffer[0] = byte1;
        buffer[1] = byte2;

        // --- 2. Read remaining 30 bytes ---
        size_t remaining_bytes = FRAME_LENGTH - 2;
        unsigned long read_timeout = millis() + 200;

        while (_serial->available() < remaining_bytes) {
          if (millis() >= read_timeout) {
            status_code = 2;  // Incomplete read/Timeout
            while (_serial->available()) _serial->read();
            if (_setPin >= 0) sleep();  // Go back to sleep on failure
            return false;
          }
        }

        if (_serial->readBytes(&buffer[2], remaining_bytes) != remaining_bytes) {
          status_code = 2;
          while (_serial->available()) _serial->read();
          if (_setPin >= 0) sleep();
          return false;
        }

        // --- 3. Check Length (0x001C = 28) ---
        uint16_t frame_len = (buffer[2] << 8) | buffer[3];
        if (frame_len != 28) {
          status_code = 2;
          while (_serial->available()) _serial->read();
          if (_setPin >= 0) sleep();
          return false;
        }

        // --- 4. Check Checksum ---
        uint16_t calculated_checksum = calculateChecksum(buffer, FRAME_LENGTH);
        uint16_t received_checksum = (buffer[FRAME_LENGTH - 2] << 8) | buffer[FRAME_LENGTH - 1];

        if (calculated_checksum != received_checksum) {
          status_code = 3;  // Checksum Error (often caused by hardware noise)
          while (_serial->available()) _serial->read();
          if (_setPin >= 0) sleep();
          return false;
        }

        // --- 5. Extract Data (Big Endian) ---
        data.pm1_0_std = (buffer[4] << 8) | buffer[5];
        data.pm2_5_std = (buffer[6] << 8) | buffer[7];
        data.pm10_0_std = (buffer[8] << 8) | buffer[9];

        data.pm1_0_atm = (buffer[10] << 8) | buffer[11];
        data.pm2_5_atm = (buffer[12] << 8) | buffer[13];
        data.pm10_0_atm = (buffer[14] << 8) | buffer[15];

        data.count_0_3um = (buffer[16] << 8) | buffer[17];
        data.count_0_5um = (buffer[18] << 8) | buffer[19];
        data.count_1_0um = (buffer[20] << 8) | buffer[21];
        data.count_2_5um = (buffer[22] << 8) | buffer[23];
        data.count_5_0um = (buffer[24] << 8) | buffer[25];
        data.count_10_0um = (buffer[26] << 8) | buffer[27];

        is_valid = true;
        if (_setPin >= 0) sleep();  // SUCCESS: Go to sleep after successful read
        return true;                // SUCCESS!
      }
    }
  }

  // If loop finishes without finding the header
  status_code = 4;  // Timeout searching for header
  while (_serial->available()) _serial->read();
  if (_setPin >= 0) sleep();
  return false;
}