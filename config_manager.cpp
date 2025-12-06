// config_manager.cpp

#include <Arduino.h> 
#include "data.h"
#include "config.h"

#include <Preferences.h>
#include <ArduinoJson.h>

AppConfig_t appConfig;

Preferences preferences;

// Default Configuration values
const AppConfig_t defaultConfig = {
    // ... (Existing network and GPIO settings)
    .wifiSSID = "HH",
    .wifiPass = "12345678",
    .mqttServer = "pi.hoan.uk",
    .mqttPort = 1883,
    .mqttUser = "weather",
    .mqttPass = "hoan1234",
    .mqttTopic = "weather/data",
    .sendInterval = 5000,  // 5 seconds
    .ntpServer = "pool.ntp.org",
    
    .dustLEDPin = 15,
    .dustADCPin = 35, 
    .mqADCPin = 34,    
    
    // --- DEFAULT MQ-135 CALIBRATION ---
    .mq_rl_kohm = 10.0,              // Load Resistor is 10kOhm
    .mq_r0_ratio_clean = 3.6,        // Baseline Rs/R0 ratio in clean air
    .tvoc_a_curve = 116.602,         // TVOC Power curve A constant
    .tvoc_b_curve = -2.769           // TVOC Power curve B constant
    // ----------------------------------
};

/**
 * @brief Resets the current configuration to default values and saves it to NVS.
 */
void resetConfig() {
    Serial.println("[CFG] Resetting config to default values...");
    // Copy the contents of defaultConfig to the global appConfig variable
    memcpy(&appConfig, &defaultConfig, sizeof(AppConfig_t));
    saveConfig();
}

/**
 * @brief Saves the current content of appConfig structure to the ESP32's NVS (Non-Volatile Storage).
 */
void saveConfig() {
    preferences.begin(PREFERENCES_NAMESPACE, false); // Read/Write access
    preferences.putBytes("config", &appConfig, sizeof(AppConfig_t));
    preferences.end();
    Serial.println("[CFG] Configuration saved successfully to NVS.");
}

/**
 * @brief Loads the configuration from NVS into the appConfig structure. If no valid config is found, it loads the default config and saves it.
 */
void loadConfig() {
    preferences.begin(PREFERENCES_NAMESPACE, true); // Read-only access
    size_t configSize = preferences.getBytesLength("config");

    // Check if the stored size matches the expected size of the structure
    if (configSize == sizeof(AppConfig_t)) {
        preferences.getBytes("config", &appConfig, sizeof(AppConfig_t));
        Serial.println("[CFG] Loaded configuration from NVS.");
    } else {
        Serial.println("[CFG] No valid configuration found in NVS. Using default.");
        // If config is missing or invalid size, reset to default and save it
        resetConfig(); 
    }
    preferences.end();
}