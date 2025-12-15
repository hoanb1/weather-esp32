// mqtt_handler.cpp
#include "mqtt_handler.h"
#include <SPIFFS.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> // Assuming you use ArduinoJson
#include "config.h"
#include "data.h"

// --- MQTT client ---
PubSubClient mqttClient(wifiClient);
static unsigned long lastQueueSend = 0;

// --- SPIFFS Status ---
static bool isSpiffsMounted = false;
static bool hasSpiffsAttempted = false; // Tracks if SPIFFS.begin() has ever been called

// --- RAM queue ---
#define MAX_RAM_QUEUE 100
static String ramQueue[MAX_RAM_QUEUE];
static uint8_t ramQueueCount = 0;

static unsigned long lastReconnectAttempt = 0;
const unsigned long RECONNECT_INTERVAL = 5000; // 5s

// =====================================================================
// JSON generator for Static Info
// =====================================================================
String getStaticInfoJson() {
    StaticJsonDocument<512> doc;
    doc["id"] = appConfig.deviceId;
    doc["type"] = "config"; // Mark as configuration payload
    doc["lat"] = appConfig.latitude;
    doc["lon"] = appConfig.longitude;
    doc["stationName"] = appConfig.stationName;
    doc["stationDescription"] = appConfig.stationDescription;

    String json;
    serializeJson(doc, json);
    return json;
}

// --- Append JSON to RAM/File queue ---
void appendToQueue(const String &json) {
    if (ramQueueCount < MAX_RAM_QUEUE) {
        ramQueue[ramQueueCount++] = json;
        addLogf("[MQTT] Added to RAM queue (%d/%d)", ramQueueCount, MAX_RAM_QUEUE);
        return;
    }

    // RAM full -> check SPIFFS first
    if (!isSpiffsMounted && !hasSpiffsAttempted) {
        hasSpiffsAttempted = true;
        if (SPIFFS.begin()) {
            isSpiffsMounted = true;
            addLog("[MQTT] SPIFFS mounted successfully.");
        } else {
            addLog("[MQTT] SPIFFS mount failed (permanently disabled for this session).");
            return;
        }
    }

    // If mounted (isSpiffsMounted == true)
    if (isSpiffsMounted) {
        // ... (File writing logic, kept brief for focus)
        File f = SPIFFS.open("/mqtt_queue.txt", "a");
        if(f) {
            for(uint8_t i = 0; i < ramQueueCount; i++) f.println(ramQueue[i]);
            f.println(json); // add new record
            f.close();
            addLogf("[MQTT] RAM queue flushed to file (%d records)", ramQueueCount+1);
            ramQueueCount = 0;
            return;
        }
    }

    addLog("[MQTT] RAM full, file operation failed, keeping data in RAM.");
}

// --- Send a single JSON safely ---
void sendMQTT(const String &json) {
    if (!mqttClient.connected()) {
        appendToQueue(json);
        return;
    }

    // Note: This function is for data topic (appConfig.mqttTopic)
    if (!mqttClient.publish(appConfig.mqttTopic, json.c_str())) {
        addLog("[MQTT] Publish failed, added to queue");
        appendToQueue(json);
    } else {
       // addLog("[MQTT] Message sent successfully");
    }
}

void sendQueue() {
    if (!mqttClient.connected()) return;

    // RAM queue first
    for(uint8_t i = 0; i < ramQueueCount; i++) {
        // Note: sendMQTT will re-queue if publish fails
        sendMQTT(ramQueue[i]);
    }
    ramQueueCount = 0;

    // Check SPIFFS and process file queue
    if (!isSpiffsMounted) return;

    if (!SPIFFS.exists("/mqtt_queue.txt")) return;

    File f = SPIFFS.open("/mqtt_queue.txt", "r");
    if(!f) return;

    File temp = SPIFFS.open("/tmp_queue.txt", "w");
    if(!temp) { f.close(); return; }

    while(f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if(line.length() == 0) continue;

        if (!mqttClient.publish(appConfig.mqttTopic, line.c_str())) {
            temp.println(line); // keep unsent lines
        }
    }

    f.close(); temp.close();
    SPIFFS.remove("/mqtt_queue.txt");
    SPIFFS.rename("/tmp_queue.txt", "/mqtt_queue.txt");
}

// --- MQTT reconnect ---
void reconnectMQTT() {
    if (!WiFi.isConnected()) return;
    if (mqttClient.connected()) return;

    unsigned long now = millis();
    if (now - lastReconnectAttempt < RECONNECT_INTERVAL) return;
    lastReconnectAttempt = now;

    addLog("[MQTT] Connecting...");

    char clientId[12];
    sprintf(clientId, "%u", appConfig.deviceId);

    // Use Unified Token for Username and Password
    const char* unifiedToken = appConfig.mqttPass;

    // =================================================================
    // START: ADDED DEBUG LOGGING
    // =================================================================
    addLogf("[DEBUG] Client ID: %s", clientId);
    addLogf("[DEBUG] Username/Password (Token): %s", unifiedToken);
    addLogf("[DEBUG] Server: %s:%d", appConfig.mqttServer, appConfig.mqttPort);
    // =================================================================
    // END: ADDED DEBUG LOGGING
    // =================================================================

    // Connect attempt: ClientID, Username (Token), Password (Token)
    if (mqttClient.connect(clientId, unifiedToken, unifiedToken)) {
        addLog("[MQTT] Connected");
        sendQueue();

        String staticInfo = getStaticInfoJson();

        // =================================================================
        // START: ADDED DEBUG LOGGING
        // =================================================================
        addLogf("[DEBUG] Config Topic: %s", MQTT_CONFIG_TOPIC);
        addLogf("[DEBUG] Config Payload: %s", staticInfo.c_str());
        // =================================================================
        // END: ADDED DEBUG LOGGING
        // =================================================================

        if (mqttClient.publish(MQTT_CONFIG_TOPIC, staticInfo.c_str())) {
            addLog("[MQTT] Static configuration info sent.");
        } else {
            addLog("[MQTT] Failed to send static configuration info.");
        }
    } else {
        addLogf("[MQTT] Connection failed, rc=%d", mqttClient.state());
    }
}

// --- Setup MQTT and Loop MQTT (Kept the same) ---
void setupMQTT() {
    if (appConfig.mqttEnabled) {
        mqttClient.setServer(appConfig.mqttServer, appConfig.mqttPort);
    }
}

void loopMQTT() {
    if (!appConfig.mqttEnabled) return;

    if (!mqttClient.connected()) reconnectMQTT();
    mqttClient.loop();

    if (millis() - lastQueueSend >= appConfig.queueFlushInterval) {
        if (mqttClient.connected()) sendQueue();
        lastQueueSend = millis();
    }
}