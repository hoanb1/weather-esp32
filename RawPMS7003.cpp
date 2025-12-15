//RawPMS7003.cpp
#include "RawPMS7003.h"

// Constructor
RawPMS7003::RawPMS7003(HardwareSerial& serial, int rxPin, int txPin)
  : _serial(&serial), _rxPin(rxPin), _txPin(txPin) {
}

// Initialization
void RawPMS7003::begin() {
  _serial->begin(9600, SERIAL_8N1, _rxPin, _txPin);
  // Give the sensor time to start up
  delay(100);
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
// File: RawPMS7003.cpp
// ... (các hàm khác giữ nguyên)

// Main read function
bool RawPMS7003::read() {
  is_valid = false;
  status_code = 0;

  // --- 1. Find Header (0x42, 0x4D) ---
  uint8_t buffer[FRAME_LENGTH];

  // Search for the start of the frame (42h 4Dh)
  // Use a short timeout for finding the start of the frame
  unsigned long timeout = millis() + 500;

  // Read and discard bytes until header is found or timeout
  while (millis() < timeout) {
    if (_serial->available()) {
      uint8_t byte = _serial->read();

      // Check for the first header byte (0x42)
      if (byte == FRAME_HEADER_1) {
        // Check for the second header byte (0x4D)
        if (_serial->available() >= 1) {
          uint8_t next_byte = _serial->read();

          if (next_byte == FRAME_HEADER_2) {
            // Header found (0x42 0x4D)
            buffer[0] = FRAME_HEADER_1;
            buffer[1] = FRAME_HEADER_2;

            // --- 2. Read remaining 30 bytes ---
            size_t remaining_bytes = FRAME_LENGTH - 2;

            // Set a short timeout for reading the rest of the frame
            unsigned long read_timeout = millis() + 50;

            // Wait until enough bytes are available
            while (_serial->available() < remaining_bytes) {
              if (millis() >= read_timeout) {
                // Timeout while reading the rest of the frame
                status_code = 2;  // Length Error / Incomplete read
                                  // Flush buffer to ensure next read starts clean
                while (_serial->available()) _serial->read();
                return false;
              }
            }

            // Read 30 bytes remaining
            if (_serial->readBytes(&buffer[2], remaining_bytes) != remaining_bytes) {
              status_code = 2;
              while (_serial->available()) _serial->read();
              return false;
            }

            // --- 3. Check Length (should be 0x001C = 28) ---
            uint16_t frame_len = (buffer[2] << 8) | buffer[3];
            if (frame_len != 28) {
              status_code = 2;                               // Length mismatch
              while (_serial->available()) _serial->read();  // Flush on length mismatch
              return false;
            }

            // --- 4. Check Checksum ---
            uint16_t calculated_checksum = calculateChecksum(buffer, FRAME_LENGTH);
            uint16_t received_checksum = (buffer[FRAME_LENGTH - 2] << 8) | buffer[FRAME_LENGTH - 1];

            if (calculated_checksum != received_checksum) {
              status_code = 3;                               // Checksum Error
              while (_serial->available()) _serial->read();  // Flush on checksum error
              return false;
            }

            // --- 5. Extract Data (Big Endian) ---
            // PM1.0 (Standard)
            data.pm1_0_std = (buffer[4] << 8) | buffer[5];
            // PM2.5 (Standard)
            data.pm2_5_std = (buffer[6] << 8) | buffer[7];
            // PM10.0 (Standard)
            data.pm10_0_std = (buffer[8] << 8) | buffer[9];

            // PM ATM concentration
            data.pm1_0_atm = (buffer[10] << 8) | buffer[11];
            data.pm2_5_atm = (buffer[12] << 8) | buffer[13];
            data.pm10_0_atm = (buffer[14] << 8) | buffer[15];

            // Particle count concentration
            data.count_0_3um = (buffer[16] << 8) | buffer[17];
            data.count_0_5um = (buffer[18] << 8) | buffer[19];
            data.count_1_0um = (buffer[20] << 8) | buffer[21];
            data.count_2_5um = (buffer[22] << 8) | buffer[23];
            data.count_5_0um = (buffer[24] << 8) | buffer[25];
            data.count_10_0um = (buffer[26] << 8) | buffer[27];

            is_valid = true;
            return true;  // SUCCESS!
          }
          // If 0x42 was followed by a non-0x4D,
          // the loop naturally continues to search for the next 0x42
        }
      }
    }
  }

  // If loop finishes without finding the header
  status_code = 4;  // Timeout searching for header
  // Flush the buffer to ensure the next read starts clean
  while (_serial->available()) _serial->read();
  return false;
}