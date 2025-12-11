// File: mqtt_handler.h
#pragma once
#include <Arduino.h>
#include <PubSubClient.h>

void setupMQTT();
void loopMQTT();
void appendToQueue(const String &json);
void sendMQTT(const String &json);  // safe MQTT send
