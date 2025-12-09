#pragma once

#include <Arduino.h>

#define LOG_BUFFER_SIZE 10
#define PREFERENCES_NAMESPACE "weather_cfg"
#define DEVICE_ID_MAX_LEN 32

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

    // Timing
    uint32_t sendInterval;
    char ntpServer[64];

    // GPIO
    uint8_t dustLEDPin;
    uint8_t dustADCPin;
    uint8_t mqADCPin;

    // MQ135 Calibration
    float mq_rl_kohm;           
    float mq_r0_ratio_clean;    
    float mq_rzero;             

    // Device Info
    char deviceId[DEVICE_ID_MAX_LEN];
    float latitude;
    float longitude;

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
