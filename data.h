//data.h
#pragma once

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <Adafruit_BME280.h>
#include <GP2YDustSensor.h>
#include "PMS7003.h"

#include "mq135.h"
#include "config.h"

// --- Global Sensor Instances ---
extern Adafruit_BME280 bme;
extern GP2YDustSensor* gp2ySensor;
extern MQ135* mq135;

// --- External Objects ---
extern AsyncWebServer server;
extern AsyncWebSocket ws;
extern WiFiClient wifiClient;
extern PubSubClient mqttClient;

extern AppConfig_t appConfig;
extern String latestJson;
extern unsigned long lastSend;
extern bool isWifiConnected;
extern bool bmeInitialized;

// --- PMS7003 ---
extern PMS7003* pms7003;

extern bool pmsInitialized;


// --- Data & Sensor Handling ---
String getDataJson();
int calcAQI_PM25(float pm25);

// --- Time ---
void setupTime();

// --- Dust Sensor ---
void initDustSensor();

// --- Web / WebSocket ---
void notifyClients(String msg);
void setupWebServer();
void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len);
void handleReboot(AsyncWebServerRequest* request);
void handleReset(AsyncWebServerRequest* request);

// --- WiFi ---
void setupWiFi();
void maintainWiFi();


// --- MQ135 ---
void initMQ135();
float calibrateMQ135();

// --- PMS7003 ---
void initPMS7003();
//MQTT
String getStaticInfoJson();