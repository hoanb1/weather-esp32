// mqtt_handler.cpp
#include "mqtt_handler.h"
#include <SPIFFS.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"

// --- MQTT client ---
static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);
static unsigned long lastQueueSend = 0;

// --- RAM queue ---
#define MAX_RAM_QUEUE 100
static String ramQueue[MAX_RAM_QUEUE];
static uint8_t ramQueueCount = 0;

static unsigned long lastReconnectAttempt = 0;
const unsigned long RECONNECT_INTERVAL = 5000; // 5s

// --- Append JSON to RAM/File queue ---
void appendToQueue(const String &json) {
    if (ramQueueCount < MAX_RAM_QUEUE) {
        ramQueue[ramQueueCount++] = json;
        addLogf("[MQTT] Added to RAM queue (%d/%d)", ramQueueCount, MAX_RAM_QUEUE);
        return;
    }

    // RAM đầy → kiểm tra SPIFFS trước
    if (SPIFFS.begin()) {  // nếu SPIFFS mount thành công
        if (!SPIFFS.exists("/mqtt_queue.txt")) {
            File f = SPIFFS.open("/mqtt_queue.txt", "w");
            if(f) f.close();
        }

        File f = SPIFFS.open("/mqtt_queue.txt", "a");
        if(f) {
            for(uint8_t i = 0; i < ramQueueCount; i++) f.println(ramQueue[i]);
            f.println(json); // thêm record mới
            f.close();
            addLogf("[MQTT] RAM queue flushed to file (%d records)", ramQueueCount+1);
            ramQueueCount = 0;
            return;
        }
    }

    // Nếu không có SPIFFS hoặc mở file fail, chỉ giữ trong RAM
    addLog("[MQTT] RAM full, cannot flush to file, keeping in RAM");
}

// --- Send a single JSON safely ---
void sendMQTT(const String &json) {
    if (!mqttClient.connected()) {
        appendToQueue(json);
        return;
    }

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
        sendMQTT(ramQueue[i]);
    }
    ramQueueCount = 0;

    // Kiểm tra SPIFFS trước khi xử lý file queue
    if (!SPIFFS.begin()) return;  // nếu không mount được, bỏ qua

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
            temp.println(line); // giữ lại các dòng chưa gửi
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
    String clientId = "ESP32Weather-" + String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str(), appConfig.mqttUser, appConfig.mqttPass)) {
        addLog("[MQTT] Connected");
        sendQueue();
    } else {
        addLogf("[MQTT] Connection failed, rc=%d", mqttClient.state());
    }
}

// --- Setup MQTT ---
void setupMQTT() {
    if (appConfig.mqttEnabled) {
        mqttClient.setServer(appConfig.mqttServer, appConfig.mqttPort);
    }
}

// --- Loop MQTT ---
void loopMQTT() {
    if (!appConfig.mqttEnabled) return;

    if (!mqttClient.connected()) reconnectMQTT();
    mqttClient.loop();

    if (millis() - lastQueueSend >= appConfig.queueFlushInterval) {
        if (mqttClient.connected()) sendQueue();
        lastQueueSend = millis();
    }
}
