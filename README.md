# ☁️ ESP32 Weather Station (weather-esp32)

This project implements a comprehensive Weather and Air Quality Station using the **ESP32** microcontroller. It is designed to read environmental data from multiple sensors, synchronize time via Network Time Protocol (NTP), and publish the results to an **MQTT Broker** while providing a **Web-based Dashboard and Configuration Portal**.

## ✨ Features

* **Multi-Sensor Data Collection:** Reads **Temperature, Humidity, Pressure** (BME280), **Dust/PM concentration** (GP2Y10), and **Gas/Air Quality** (MQ-135).
* **Advanced Gas Processing:** Automatically converts the raw ADC reading from the MQ-135 into an estimated **TVOC concentration ($\text{PPM}$)** using configurable calibration parameters ($R_L$, $R_O$ ratio, Power Curve A/B).
* **Combined AQI Calculation:** Calculates a **Total Air Quality Index (AQI)** by taking the maximum of the individual indices for $\mathbf{PM}_{2.5}$ and **TVOC (Indoor Air Quality)**, providing a more comprehensive environmental score.
* **Persistent & Flexible Configuration:** Stores WiFi, MQTT, operational settings, **sensor GPIO assignments, and MQ-135 calibration values** in the ESP32's **NVS (Non-Volatile Storage)**, configurable via the web interface.
* **Smart WiFi Management:** Scans for the configured SSID and connects to the **strongest node/BSSID** for improved stability, with a fallback to **Access Point (AP) mode** if connection fails.
* **MQTT Integration:** Publishes detailed JSON data payloads to a configurable MQTT Topic, designed to integrate seamlessly with platforms like Home Assistant or Node-RED.
* **Live Web Dashboard:** Provides a responsive, real-time web interface using **WebSockets** and **Chart.js** for live data visualization of all measured and calculated values.
* **Automatic Baseline Adjustment:** Gradually adjusts the dust sensor's baseline (downwards only) to adapt to clean environments over time.

---

## 🛠️ Components & Requirements

| Component | Description | Integration |
| :--- | :--- | :--- |
| **Microcontroller** | ESP32 (Any variant) | WiFi, NVS, I2C, ADC |
| **BME280** | Temperature, Humidity, Pressure Sensor | I2C |
| **GP2Y10** | Analog Dust Sensor (PM approximation) | **Configurable** Analog Input, **Configurable** GPIO (for LED control) |
| **MQ-135** (or similar) | Gas/Air Quality Sensor (TVOC/CO2 equivalent) | **Configurable** Analog Input (ADC) |
| **Software** | AsyncWebServer, PubSubClient, Adafruit BME280, GP2YDustSensor, ArduinoJson | Libraries |

---

## ⚙️ Setup and Configuration

The device is designed for easy initial setup via its built-in Web Server.

1.  **Initial Power-Up:** The ESP32 will first try to connect using saved credentials. If no valid configuration is found, or if connection fails, it starts in **Access Point (AP) Mode**.
2.  **Connect to AP:** Connect your phone or PC to the WiFi network named **`ESP32-Weather-AP`** (Password: `12345678`).
3.  **Access Settings:** Navigate to `http://192.168.4.1/settings` in your browser.
4.  **Configure:** Enter your network credentials, MQTT server details, data `sendInterval`, **và các tham số hiệu chuẩn cho MQ-135 (RL, Rs/R0 Baseline, TVOC Curve)**.
5.  **Save & Reboot:** Click "Save & Reboot" to store the configuration in NVS and restart the device.

---

## 💻 Code Structure (Updated)

| File | Role | Key Functions |
| :--- | :--- | :--- |
| **`weather-esp32.ino`** | Main logic, `setup()` and `loop()`. | Initial device and service setup, data transmission loop, MQTT maintenance. |
| **`config.h`** | Configuration definition. | Defines the `AppConfig_t` structure, including **GPIO pins and MQ calibration floats**. |
| **`config_manager.cpp`** | NVS handling. | `loadConfig()`, `saveConfig()`, `resetConfig()`. |
| **`data.h`** | Global declarations. | Declares sensor objects, global variables, and function prototypes. |
| **`data_sensor.cpp`** | Sensor reading & calculation. | `setupTime()`, `initDustSensor()`, **`calculateRsRoRatio()`**, **`calculateTVOC_PPM()`**, **`calcAQI_PM25()`**, **`calcAQI_TVOC()`**, `getDataJson()`. |
| **`wifi_manager.cpp`** | Network connectivity. | `setupWiFi()`, `connectToStrongestNode()`, `reconnectMQTT()`. |
| **`web_server.cpp`** | Web Interface. | `setupWebServer()`, `getSettingsPage()` (Handles MQ calibration inputs), `onWsEvent()` (Dashboard & Config). |

---

## 📈 Data Payload Format (Detailed Output)

Data is published to the MQTT broker and WebSocket clients as a JSON string, including the calculated TVOC value and the combined AQI.

```json
{
  "temp": 28.5,
  "humid": 65.2,
  "pressure": 1012.3,
  "dust": 15,
  "mq135_raw": 450,       // Raw ADC value (for debugging)
  "mq_rs_ro": 1.56,       // Calculated Rs/R0 ratio
  "mq_tvoc_ppm": 850,     // Estimated TVOC concentration (PPM)
  "aqi": 58,              // Combined AQI (max of PM2.5 and TVOC)
  "aqi_pm25": 58,         // Individual AQI for PM2.5
  "aqi_tvoc": 55,         // Individual AQI for TVOC/IAQ
  "wifiRssi": -65,
  "createdAt": 1678886400123456
}