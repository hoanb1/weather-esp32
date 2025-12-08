#pragma once

#include <Arduino.h>

#define LOG_BUFFER_SIZE 10
#define PREFERENCES_NAMESPACE "weather_cfg"
#define DEVICE_ID_MAX_LEN 32

// Application Configuration Structure
typedef struct {
  // WiFi
  char wifiSSID[32];
  char wifiPass[64];

  // MQTT
  char mqttServer[64];
  uint16_t mqttPort;
  char mqttUser[32];
  char mqttPass[64];
  char mqttTopic[64];

  // Timing
  uint32_t sendInterval;      // milliseconds
  char ntpServer[64];

  // GPIO
  uint8_t dustLEDPin;
  uint8_t dustADCPin;
  uint8_t mqADCPin;

  // MQ135 Calibration
  float mq_rl_kohm;           // Load resistor (kΩ)
  float mq_r0_ratio_clean;    // Rs/R0 in clean air (≈3.6)
  float tvoc_a_curve;
  float tvoc_b_curve;
  float mq_rzero;

  float co2_a_curve;
  float co2_b_curve;
  float nh3_a_curve;
  float nh3_b_curve;
  float alcohol_a_curve;
  float alcohol_b_curve;

  // Device Info
  char deviceId[DEVICE_ID_MAX_LEN];
  float latitude;
  float longitude;

} AppConfig_t;

extern AppConfig_t appConfig;

// Config Management
void loadConfig();
void saveConfig();
void resetConfig();

// Logging
struct LogEntry {
  String message;
};

extern LogEntry logBuffer[LOG_BUFFER_SIZE];
extern uint8_t logIndex;

void addLogf(const char* format, ...);
void addLog(const char* msg);
