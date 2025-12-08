# ESP32 Weather Station (weather-esp32)

This project implements a comprehensive **Weather and Air Quality Station** using the **ESP32** microcontroller. It is designed to read environmental data from multiple sensors, synchronize time via Network Time Protocol (NTP), and publish the results to an **MQTT Broker** while providing a **Web-based Dashboard and Configuration Portal**.

---

## Features

* **Multi-Sensor Data Collection:** Reads **Temperature, Humidity, Pressure** (BME280), **Dust/PM concentration** (GP2Y10), and **Gas/Air Quality** (MQ-135 or similar).
* **Advanced Gas Processing:** Uses the **MQ-135** sensor and incorporates a library/function to calculate a **Corrected Air Quality concentration ($\text{PPM}$)**, factoring in ambient temperature and humidity for better accuracy.
* **Combined AQI Calculation:** Calculates a **Total Air Quality Index (AQI)** based on the **$\mathbf{PM}_{2.5}$** concentration from the dust sensor.
* **Persistent & Flexible Configuration:** Stores WiFi, MQTT, operational settings, **sensor GPIO assignments, and MQ-135 calibration values** in the ESP32's **NVS (Non-Volatile Storage)**, configurable via the web interface.
* **Smart WiFi Management:** Scans for the configured SSID and connects to the **strongest node/BSSID** for improved stability, with a fallback to **Access Point (AP) mode** if connection fails.
* **MQTT Integration:** Publishes detailed JSON data payloads to a configurable MQTT Topic, designed to integrate seamlessly with platforms like Home Assistant or Node-RED.
* **Live Web Dashboard:** Provides a responsive, real-time web interface using **WebSockets** and **Chart.js** for live data visualization of all measured and calculated values, including a real-time log.

---

## 🛠️ Components & Requirements

| Component | Description | Integration |
| :--- | :--- | :--- |
| **Microcontroller** | ESP32 (Any variant) | WiFi, NVS, I2C, ADC |
| **BME280** | Temperature, Humidity, Pressure Sensor | I2C |
| **GP2Y10** | Analog Dust Sensor (PM approximation) | **Configurable** Analog Input, **Configurable** GPIO (for LED control) |
| **MQ-135** (or similar) | Gas/Air Quality Sensor (TVOC/CO2 equivalent) | **Configurable** Analog Input (ADC) |
| **Software** | AsyncWebServer, PubSubClient, Adafruit BME280, GP2YDustSensor, ArduinoJson (Libraries) | Required Libraries |

---

## ⚙️ Setup and Configuration

The device is designed for easy initial setup via its built-in Web Server.

1.  **Initial Power-Up:** The ESP32 will first try to connect using saved credentials. If no valid configuration is found, or if connection fails, it starts in **Access Point (AP) Mode**.
2.  **Connect to AP:** Connect your phone or PC to the WiFi network named **`ESP32-Weather-AP`** (Password: `12345678`).
3.  **Access Settings:** Navigate to `http://192.168.4.1/settings` in your browser.
4.  **Configure:** Enter your network credentials, MQTT server details, data `sendInterval`, **and the calibration parameters for the MQ-135 (RL, $\text{Rs/R}_0$ Baseline, TVOC Curve)**.
5.  **Save & Reboot:** Click "Save & Reboot" to store the configuration in NVS and restart the device.

---

## 💻 Code Structure

| File | Role | Key Functions |
| :--- | :--- | :--- |
| **`weather-esp32.ino`** | Main logic, `setup()` and `loop()`. | Initial device and service setup, data transmission loop, MQTT maintenance. |
| **`config.h`** | Configuration definition. | Defines the `AppConfig_t` structure, including **GPIO pins and MQ calibration floats**. |
| **`config_manager.cpp`** | NVS and Logging handling. | `loadConfig()`, `saveConfig()`, `resetConfig()`, `addLog()`, `addLogf()`. |
| **`data.h`** | Global declarations. | Declares sensor objects, global variables, and function prototypes. |
| **`data_sensor.cpp`** | Sensor reading & calculation. | `setupTime()`, `initDustSensor()`, **`getDataJson()`** (Collects all data), **`calcAQI_PM25()`**. |
| **`wifi_manager.cpp`** | Network connectivity. | `setupWiFi()`, `connectToStrongestNode()`, `reconnectMQTT()`. |
| **`web_server.cpp`** | Web Interface. | `setupWebServer()`, `getSettingsPage()` (Handles MQ calibration inputs), `onWsEvent()` (Dashboard & Config). |

---

## 📈 Data Payload Format (MQTT/WebSocket)

Data is published to the MQTT broker and WebSocket clients as a JSON string. The current implementation publishes a concise set of key values.

```json
{
  "id": "01",              // Device ID
  "t": 28.5,               // Temperature (°C)
  "h": 65.2,               // Humidity (%)
  "p": 1012.3,             // Pressure (hPa)
  "pm": 15,                // Dust/PM raw density (approximation)
  "mqr": 450,              // MQ135 ADC raw value (for debugging)
  "mqp": 850,              // Corrected Air Quality concentration (PPM)
  "aqi": 58,               // Total AQI (from PM2.5)
  "ts": 1678886400123456   // Timestamp (microseconds)
}