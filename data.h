//data.h
#pragma once

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <Adafruit_BME280.h>
#include <GP2YDustSensor.h>

#include "config.h"
#include "MQ135_ESP32.h"

// -----------------------------------------------------------------------------
// External Objects
// -----------------------------------------------------------------------------
extern AsyncWebServer server;
extern AsyncWebSocket ws;

extern WiFiClient espClient;
extern PubSubClient mqttClient;

extern Adafruit_BME280 bme;
extern GP2YDustSensor* dustSensor;
extern MQ135_ESP32* mq135;

extern AppConfig_t appConfig;
extern String latestJson;
extern unsigned long lastSend;
extern bool isWifiConnected;
extern bool bmeInitialized;

// -----------------------------------------------------------------------------
// Data & Sensor Handling
// -----------------------------------------------------------------------------
String getDataJson();
int calcAQI(float pm25);

// -----------------------------------------------------------------------------
// Time
// -----------------------------------------------------------------------------
void setupTime();

// -----------------------------------------------------------------------------
// Dust Sensor
// -----------------------------------------------------------------------------
void initDustSensor();

// -----------------------------------------------------------------------------
// Web / WebSocket
// -----------------------------------------------------------------------------
void notifyClients(String msg);
void setupWebServer();
void onWsEvent(AsyncWebSocket* server,
                AsyncWebSocketClient* client,
                AwsEventType type,
                void* arg,
                uint8_t* data,
                size_t len);

void handleReboot(AsyncWebServerRequest* request);
void handleReset(AsyncWebServerRequest* request);

// -----------------------------------------------------------------------------
// WiFi
// -----------------------------------------------------------------------------
void setupWiFi();
void maintainWiFi();

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------
void reconnectMQTT();

// -----------------------------------------------------------------------------
// MQ135
// -----------------------------------------------------------------------------
void initMQ135();
void startMQ135Calibration();
void processMQ135Calibration(MQ135_ESP32& sensor);
