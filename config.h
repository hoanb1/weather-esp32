// config.h
#pragma once

#include <Arduino.h>

#define LOG_BUFFER_SIZE 10
#define PREFERENCES_NAMESPACE "weather_cfg"


// --- App Configuration ---
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
  bool mqttEnabled;

  uint32_t queueMaxSize;        // max file size in bytes
  uint16_t queueFlushInterval;  // attempt to send every X ms

  // Timing
  uint32_t sendInterval;
  char ntpServer[64];

  // GPIO
  uint8_t dustLEDPin;
  uint8_t dustADCPin;
  uint8_t mqADCPin;

  uint8_t pmsRxPin; // Pin 27
  uint8_t pmsTxPin; // Pin 26
  int pmsSetPin; // Pin 26

  // MQ135 Calibration
  float mq_rl_kohm;
  float mq_r0_ratio_clean;
  float mq_rzero;
  float dust_baseline;
  float dust_calibration = 1.0f;


  bool autoCalibrateOnBoot;

  // Device Info
  uint32_t deviceId;
  float latitude;
  float longitude;
  char stationName[64];
  char stationDescription[128];

} AppConfig_t;

// --- Global Config Instance ---
extern AppConfig_t appConfig;

// --- Config Management ---
void loadConfig();
void saveConfig();
void resetConfig();

// --- Logging ---
struct LogEntry {
  String message;
};

extern LogEntry logBuffer[LOG_BUFFER_SIZE];
extern uint8_t logIndex;

void addLog(const char* msg);
void addLogf(const char* format, ...);
