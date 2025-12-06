// data.h
#pragma once

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <Adafruit_BME280.h>
#include <GP2YDustSensor.h>
#include "config.h"

// ================= CONFIG & GLOBAL VARIABLES ==================
extern AppConfig_t appConfig;
extern String latestJson;
extern unsigned long lastSend;
extern bool isWifiConnected;
extern bool bmeInitialized;



// ================= HARDWARE & SENSOR DECLARATIONS ==================

extern WiFiClient espClient;
extern PubSubClient mqttClient;
extern AsyncWebServer server;
extern AsyncWebSocket ws;
extern Adafruit_BME280 bme;

// MODIFIED: Declared as a pointer, initialized in data_sensor.cpp's initDustSensor
extern GP2YDustSensor *dustSensor; 

// ================= FUNCTION PROTOTYPES ==================

// Prototypes for data_sensor.cpp
String getDataJson();
void setupTime();
void initDustSensor(); // NEW: Used to initialize dustSensor after config load
int calcAQI(float pm25);

// Prototypes for web_server.cpp
void notifyClients(String msg);
void setupWebServer();
void handleReboot(AsyncWebServerRequest *request);
void handleReset(AsyncWebServerRequest *request);

// Prototypes for wifi_manager.cpp
void setupWiFi();
void maintainWiFi();
void reconnectMQTT();

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);