// config.h
#pragma once

#include <Arduino.h>

#define LOG_BUFFER_SIZE 10
#define PREFERENCES_NAMESPACE "weather_cfg"


// --- App Configuration ---
typedef struct {
  // ------------------------------------
  // I. GENERAL & DEVICE INFO (Đọc/Ghi)
  // ------------------------------------
  uint32_t deviceId;          // Device ID (Auto-assigned, Read-only)
  float latitude;
  float longitude;
  char stationName[64];
  char stationDescription[128];

  // ------------------------------------
  // II. NETWORK & WIFI
  // ------------------------------------
  char wifiSSID[32];
  char wifiPass[64];

  // ------------------------------------
  // III. MQTT & DATA SERVICE
  // ------------------------------------
  bool mqttEnabled;           // Enable/Disable MQTT
  char mqttServer[64];
  uint16_t mqttPort;
  char mqttPass[64];          // Unified Token / API Key (Used as Username/Password)

  uint32_t queueMaxSize;        // Max file size for queue (bytes)
  uint16_t queueFlushInterval;  // Attempt to send queue every X ms

  // ------------------------------------
  // IV. TIMING & POWER
  // ------------------------------------
  uint32_t sendInterval;      // Data send interval (ms)
  char ntpServer[64];
  int timeZone;               // Time Zone offset (hours, e.g., 7 for Vietnam)
  bool enableSleep;           // Enable deep sleep/light sleep mode
  uint32_t sleepDuration;     // Sleep duration (seconds)

  // ------------------------------------
  // V. SENSOR PINS & CONFIG
  // ------------------------------------
  // Dust (GP2Y1010AU0F)
  uint8_t dustLEDPin;
  uint8_t dustADCPin;

  // Gas (MQ-X)
  uint8_t mqADCPin;

  // Particulate Matter (PMS7003/PMSA003)
  bool pmsEnabled;            // Enable/Disable PMS Sensor
  uint8_t pmsRxPin; // Pin 17
  uint8_t pmsTxPin; // Pin 16
  int pmsSetPin;              // Control pin (Set/Wake up) - Pin -1 or valid pin
  
  // ------------------------------------
  // VI. SENSOR CALIBRATION & OFFSET
  // ------------------------------------
  // MQ Calibration
  float mq_rl_kohm;           // Load resistor value in kOhm
  float mq_r0_ratio_clean;    // R0/Rs ratio in clean air (e.g., 3.6 for MQ135)
  float mq_rzero;             // Calculated R0 value (persisted state)

  // Dust Calibration
  float dust_baseline;        // Sensor baseline voltage (V) (persisted state)
  float dust_calibration;     // Dust calibration factor (default 1.0)

  bool autoCalibrateOnBoot;   // Perform calibration on boot


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
