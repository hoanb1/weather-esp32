// config.h
#pragma once

#include <Arduino.h>

#define PREFERENCES_NAMESPACE "weather_cfg"

typedef struct {
    char wifiSSID[32];
    char wifiPass[64];
    char mqttServer[64];
    uint16_t mqttPort;
    char mqttUser[32];
    char mqttPass[64];
    char mqttTopic[64];
    uint32_t sendInterval; // Unit: milliseconds
    char ntpServer[64];
    
    // --- GPIO CONFIGURATION ---
    uint8_t dustLEDPin;    
    uint8_t dustADCPin;    
    uint8_t mqADCPin;      
    
    // --- MQ-135 CALIBRATION PARAMETERS (Float values must be stored as int/uint32 for NVS compatibility, 
    // but for simplicity in this AppConfig_t, we will assume float is supported or handled externally for now) ---
    float mq_rl_kohm;           // Load Resistor Resistance (kOhms)
    float mq_r0_ratio_clean;    // Rs/R0 ratio in clean air (Baseline value, typically ~3.6)
    float tvoc_a_curve;         // TVOC power curve constant A
    float tvoc_b_curve;         // TVOC power curve constant B

} AppConfig_t;

extern AppConfig_t appConfig;

void loadConfig();
void saveConfig();
void resetConfig();