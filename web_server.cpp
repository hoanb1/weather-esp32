// web_server.cpp

#include <Arduino.h>            // Defines String, Serial, delay, ESP.restart, v.v.
#include <WiFi.h>               // Defines WiFi
#include <AsyncTCP.h>           // Required for ESPAsyncWebServer
#include <ESPAsyncWebServer.h>  // Defines AsyncWebServer, AsyncWebServerRequest, ws, server, HTTP_GET, HTTP_POST
#include <ArduinoJson.h>        // Defines StaticJsonDocument, DeserializationError
#include <cstdint>              // Defines uint8_t, uint16_t, uint32_t
#include <cstring>              // Defines strncpy
#include <cstddef>              // Defines NULL and size_t

// Internal headers
#include "config.h"  // For AppConfig_t and appConfig
#include "data.h"    // For global declarations like ws, server, onWsEvent, isWifiConnected

void handleReboot(AsyncWebServerRequest * request) {
  request->send(200, "text/plain", "Rebooting device...");
  Serial.println("Web: Manual reboot requested. Restarting...");
  delay(1000);
  ESP.restart();
}

/**
 * @brief Handles the /reset request by restoring factory settings and restarting.
 * This definition MUST be outside any other function.
 */
void handleReset(AsyncWebServerRequest * request) {
  // NOTE: resetConfig() must be defined/declared elsewhere (e.g., config_manager.cpp)
  request->send(200, "text/plain", "Factory settings restored. Rebooting...");
  Serial.println("Web: Factory reset requested. Erasing NVS and restarting...");
  resetConfig();  // Assumes this function erases NVS
  delay(1000);
  ESP.restart();
}

// HTML for the configuration page
String getSettingsPage() {
  // Use ArduinoJson to create a JSON string of the current config for JavaScript
  StaticJsonDocument<512> doc;
  doc["wifiSSID"] = appConfig.wifiSSID;
  // NOTE: Passwords are not included in the JSON for security when loading the form.
  doc["mqttServer"] = appConfig.mqttServer;
  doc["mqttPort"] = appConfig.mqttPort;
  doc["mqttUser"] = appConfig.mqttUser;
  doc["mqttTopic"] = appConfig.mqttTopic;
  doc["sendInterval"] = appConfig.sendInterval;
  doc["ntpServer"] = appConfig.ntpServer;

  // --- ADDED: SENSOR PINS ---
  doc["dustLEDPin"] = appConfig.dustLEDPin;
  doc["dustADCPin"] = appConfig.dustADCPin;
  doc["mqADCPin"] = appConfig.mqADCPin;
  
  // --- ADDED: MQ Calibration Parameters ---
  doc["mq_rl_kohm"] = appConfig.mq_rl_kohm;
  doc["mq_r0_ratio_clean"] = appConfig.mq_r0_ratio_clean;
  doc["tvoc_a_curve"] = appConfig.tvoc_a_curve;
  doc["tvoc_b_curve"] = appConfig.tvoc_b_curve;

  String jsonConfig;
  serializeJson(doc, jsonConfig);

  // Status message display logic
  String statusColor = isWifiConnected ? "green" : "red";
  String statusIP = isWifiConnected ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  String statusMsg = "<span style='color:" + statusColor + ";'>" + (isWifiConnected ? "Connected: " : "Hotspot Mode: ") + statusIP + "</span>";

  // Raw HTML content for the settings page (using R"rawliteral(...)")
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <style>
        body{font-family:Arial,sans-serif;background:#f4f7f6;padding:20px;color:#333;}
        .container{max-width:500px;margin:0 auto;background:#fff;padding:20px;border-radius:10px;box-shadow:0 4px 10px rgba(0,0,0,0.1);}
        h2{color:#2c3e50;border-bottom:2px solid #3498db;padding-bottom:10px;}
        h3{border-top: 1px dashed #ccc; padding-top: 10px;}
        label{display:block;margin-top:10px;font-weight:bold;}
        input[type=text],input[type=password],input[type=number]{width:calc(100% - 20px);padding:10px;margin-top:5px;border:1px solid #ccc;border-radius:5px;}
        button{background:#2ecc71;color:white;padding:10px 15px;border:none;border-radius:5px;cursor:pointer;margin-top:20px;font-size:16px;}
        button:hover{background:#27ae60;}
        .button-group button{margin-right:10px;}
        .warning-btn{background:#e74c3c;}
        .warning-btn:hover{background:#c0392b;}
        .status{margin-bottom:20px;font-weight:bold;padding:10px;border-radius:5px;background:#ecf0f1;}
    </style>
</head>
<body>
    <div class="container">
        <h2>System Configuration</h2>
        <div class="status">Current Status: )rawliteral"
                + statusMsg + R"rawliteral(</div>
        <form id="configForm">
            <h3>WiFi Settings</h3>
            <label for="wifiSSID">WiFi SSID:</label>
            <input type="text" id="wifiSSID" name="wifiSSID" required>
            <label for="wifiPass">WiFi Password (Leave blank to keep current password):</label>
            <input type="password" id="wifiPass" name="wifiPass">

            <h3>MQTT Settings</h3>
            <label for="mqttServer">Server:</label>
            <input type="text" id="mqttServer" name="mqttServer" required>
            <label for="mqttPort">Port:</label>
            <input type="number" id="mqttPort" name="mqttPort" required>
            <label for="mqttUser">Username:</label>
            <input type="text" id="mqttUser" name="mqttUser">
            <label for="mqttPass">Password (Leave blank to keep current password):</label>
            <input type="password" id="mqttPass" name="mqttPass">
            <label for="mqttTopic">Topic:</label>
            <input type="text" id="mqttTopic" name="mqttTopic" required>

            <h3>Data & Timing Settings</h3>
            <label for="sendInterval">Send Interval (ms):</label>
            <input type="number" id="sendInterval" name="sendInterval" required>
            <label for="ntpServer">NTP Server:</label>
            <input type="text" id="ntpServer" name="ntpServer" required>
            
            <h3>Sensor GPIO Pins</h3>
            <label for="dustLEDPin">Dust Sensor LED Pin (GPIO):</label>
            <input type="number" id="dustLEDPin" name="dustLEDPin" required>
            <label for="dustADCPin">Dust Sensor Analog Pin (GPIO):</label>
            <input type="number" id="dustADCPin" name="dustADCPin" required>
            <label for="mqADCPin">MQ-135 Analog Pin (GPIO):</label>
            <input type="number" id="mqADCPin" name="mqADCPin" required>

            <h3>MQ-135 Calibration (TVOC)</h3>
            <label for="mq_rl_kohm">Load Resistor (RL, kΩ):</label>
            <input type="number" step="0.01" id="mq_rl_kohm" name="mq_rl_kohm" required>
            <label for="mq_r0_ratio_clean">Clean Air Rs/R0 Ratio (Baseline):</label>
            <input type="number" step="0.001" id="mq_r0_ratio_clean" name="mq_r0_ratio_clean" required>
            <label for="tvoc_a_curve">PPM Curve A (Slope):</label>
            <input type="number" step="0.001" id="tvoc_a_curve" name="tvoc_a_curve" required>
            <label for="tvoc_b_curve">PPM Curve B (Exponent):</label>
            <input type="number" step="0.001" id="tvoc_b_curve" name="tvoc_b_curve" required>

            <button type="submit">Save & Reboot</button>
        </form>

        <div class="button-group">
            <button onclick="window.location.href='/'">View Dashboard</button>
            <button class="warning-btn" onclick="if(confirm('Are you sure you want to reboot the device?')) window.location.href='/reboot'">Reboot Device</button>
            <button class="warning-btn" onclick="if(confirm('WARNING: This will erase ALL saved settings and reboot. Are you sure?')) window.location.href='/reset'">Factory Reset</button>
        </div>
    </div>
    <script>
        // Load configuration data
        const configData = JSON.parse(')rawliteral"
                + jsonConfig + R"rawliteral(');
        document.getElementById('wifiSSID').value = configData.wifiSSID;
        document.getElementById('mqttServer').value = configData.mqttServer;
        document.getElementById('mqttPort').value = configData.mqttPort;
        document.getElementById('mqttUser').value = configData.mqttUser;
        document.getElementById('mqttTopic').value = configData.mqttTopic;
        document.getElementById('sendInterval').value = configData.sendInterval;
        document.getElementById('ntpServer').value = configData.ntpServer;
        
        // Load GPIO Pins
        document.getElementById('dustLEDPin').value = configData.dustLEDPin;
        document.getElementById('dustADCPin').value = configData.dustADCPin;
        document.getElementById('mqADCPin').value = configData.mqADCPin;

        // Load MQ Calibration settings
        document.getElementById('mq_rl_kohm').value = configData.mq_rl_kohm;
        document.getElementById('mq_r0_ratio_clean').value = configData.mq_r0_ratio_clean;
        document.getElementById('tvoc_a_curve').value = configData.tvoc_a_curve;
        document.getElementById('tvoc_b_curve').value = configData.tvoc_b_curve;

        document.getElementById('configForm').onsubmit = function(e) {
            e.preventDefault();
            
            // Collect data
            const formData = new FormData(this);
            const data = {};
            for (const [key, value] of formData.entries()) {
                // Parse float values correctly
                if (['mq_rl_kohm', 'mq_r0_ratio_clean', 'tvoc_a_curve', 'tvoc_b_curve'].includes(key)) {
                    data[key] = parseFloat(value);
                } else if (['mqttPort', 'sendInterval', 'dustLEDPin', 'dustADCPin', 'mqADCPin'].includes(key)) {
                    // Parse pin numbers and intervals as integers
                    data[key] = parseInt(value); 
                } else {
                    data[key] = value;
                }
            }

            // Simple validation
            if (data.sendInterval < 1000) { 
                alert("Send Interval must be at least 1000ms (1s).");
                return;
            }

            // Handle password fields: only send if not blank
            if (data.wifiPass === "") delete data.wifiPass;
            if (data.mqttPass === "") delete data.mqttPass;

            // Send data to server
            fetch('/save', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(data)
            })
            .then(response => response.text())
            .then(text => {
                alert('Success: ' + text);
                setTimeout(() => window.location.href='/', 1000);
            })
            .catch(error => {
                alert('Error: ' + error);
            });
        };
    </script>
</body>
</html>
)rawliteral";
  return html;
}

/**
 * @brief Sets up the asynchronous web server routes and WebSocket handler.
 */
void setupWebServer() {
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // Dashboard Page (Root /)
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    // HTML content for the dashboard with Chart.js integration
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>ESP32 Weather Monitor</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>
<script src='https://cdn.jsdelivr.net/npm/luxon@3/build/global/luxon.min.js'></script>
<script src='https://cdn.jsdelivr.net/npm/chartjs-adapter-luxon@1'></script>

<style>
/* ... (Styles unchanged) ... */
body{font-family:Arial, sans-serif;margin:0;padding:0;background:#f2f2f2;color:#333;}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));grid-gap:15px;padding:15px;}
@media (min-width: 960px) {.grid{grid-template-columns:repeat(3,1fr);}}
.card{background:white;padding:10px;border-radius:10px;box-shadow:0 4px 8px rgba(0,0,0,0.1);text-align:center;transition: transform 0.2s;}
.card:hover {transform: translateY(-2px);}
.value{font-size:32px;font-weight:bold;margin-top:5px;color:#2c3e50;} 
canvas{width:100% !important;height:140px !important;}
nav{padding:10px;background:#2c3e50;color:white;text-align:center;box-shadow:0 2px 4px rgba(0,0,0,0.1);display:flex;justify-content:space-between;align-items:center;}
nav h1{margin:0;font-size:1.5em;flex-grow:1;}
nav a{color:white;margin:0 10px;text-decoration:none;font-weight:bold;padding:5px 10px;border:1px solid white;border-radius:5px;}
</style>
</head>
<body>
<nav><h1>ESP32 Weather Monitor</h1><a href="/settings">Settings</a></nav>
<div class='grid'>
<div class='card'><h4>Temp / Humidity (°C/%)</h4><div id='tempHumVal' class='value'>--</div><canvas id='chartTempHum'></canvas></div>
<div class='card'><h4>Pressure (hPa)</h4><div id='presVal' class='value'>--</div><canvas id='chartPres'></canvas></div>
<div class='card'><h4>Dust (ug/m3)</h4><div id='dustVal' class='value'>--</div><canvas id='chartDust'></canvas></div>
<div class='card'><h4>TVOC (PPM)</h4><div id='mqVal' class='value'>--</div><canvas id='chartMQ'></canvas></div> 
<div class='card' id='aqiCard' style="border-top: 5px solid #ccc;"><h4>Combined AQI</h4><div id='aqiVal' class='value'>--</div><canvas id='chartAQI'></canvas></div>
<div class='card'><h4>WiFi RSSI (dBm)</h4><div id='wifiVal' class='value'>--</div><canvas id='chartWiFi'></canvas></div>
</div>
<script>
let labels=[], tempHumData=[], presData=[], dustData=[], mqData=[], aqiData=[], wifiData=[];
const maxPoints=120; // Data resampling: 120 points (e.g., 10 minutes at 5s interval)

function addData(arr,v){arr.push(v);if(arr.length>maxPoints) arr.shift();}
function getAQIColor(aqi){if(aqi<=50)return'#00e400';else if(aqi<=100)return'#ffff00';else if(aqi<=150)return'#ff7e00';else if(aqi<=200)return'#ff0000';else if(aqi<=300)return'#99004c';else return'#7e0023';}

// Chart creation function
function createChart(ctx,dataArray,color, beginAtZero=false){
  return new Chart(ctx,{
    type:'line',
    data:{
      labels:labels,
      datasets:[{
        data:dataArray,
        borderColor:color,
        fill:false,
        tension: 0.4, 
        pointRadius: 0, 
        borderWidth: 1.5 
      }]
    },
    options:{
      animation:{duration:0},
      layout: {padding: {top: 10, bottom: 0, left: 0, right: 10}}, 
      plugins:{
        legend:{display:false},
        title:{display:false}
      },
      scales:{
        x:{
          type:'time',
          time:{unit:'second', displayFormats: {second: 'HH:mm:ss'}},
          grid: {display: false},
          ticks: {display: false} 
        },
        y:{
          beginAtZero: beginAtZero, 
          grid: {color: 'rgba(0,0,0,0.1)'},
          ticks: {font: {size: 10}}
        }
      },
      maintainAspectRatio: false
    }
  });
}

// Initializing charts
const chartTempHum=createChart(document.getElementById('chartTempHum').getContext('2d'),tempHumData,'#e74c3c', false);
const chartPres=createChart(document.getElementById('chartPres').getContext('2d'),presData,'#3498db', false);
const chartDust=createChart(document.getElementById('chartDust').getContext('2d'),dustData,'#964B00', true);
const chartMQ=createChart(document.getElementById('chartMQ').getContext('2d'),mqData,'#f39c12', true);
const chartAQI=createChart(document.getElementById('chartAQI').getContext('2d'),aqiData,'#8e44ad', true);
const chartWiFi=createChart(document.getElementById('chartWiFi').getContext('2d'),wifiData,'#2ecc71', false);

let ws=new WebSocket('ws://'+location.hostname+'/ws');
ws.onmessage=e=>{
  const d=JSON.parse(e.data);
  document.getElementById('tempHumVal').innerText=d.temp.toFixed(1)+'°C / '+d.humid.toFixed(1)+'%';
  document.getElementById('presVal').innerText=d.pressure.toFixed(1);
  document.getElementById('dustVal').innerText=d.dust.toFixed(1);
  document.getElementById('mqVal').innerText=d.mq_tvoc_ppm; // FIX: Use mq_tvoc_ppm
  document.getElementById('aqiVal').innerText=d.aqi;
  document.getElementById('aqiCard').style.borderTop='5px solid '+getAQIColor(d.aqi);
  document.getElementById('wifiVal').innerText=d.wifiRssi+' dBm';

  // Add data points
  // Convert microseconds (d.createdAt) to milliseconds for Date object
  labels.push(new Date(Number(d.createdAt) / 1000)); 
  if(labels.length>maxPoints)labels.shift();
  
  addData(tempHumData,d.temp);
  addData(presData,d.pressure);
  addData(dustData,d.dust);
  addData(mqData,d.mq_tvoc_ppm); // FIX: Use mq_tvoc_ppm
  addData(aqiData,d.aqi);
  addData(wifiData,d.wifiRssi);

  // Update charts
  chartTempHum.update();
  chartPres.update();
  chartDust.update();
  chartMQ.update();
  chartAQI.update();
  chartWiFi.update();
};
</script>
</body>
</html>
)rawliteral";
    request->send(200, "text/html; charset=utf-8", html);
  });

  // Configuration Page
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html; charset=utf-8", getSettingsPage());
  });

  // Handle configuration saving (POST request)
 server.on(
    "/save", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (index == 0) Serial.println("Web: Receiving config update...");

      // Parse JSON payload
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, (const char *)data, len);

      if (error) {
        Serial.printf("Web: JSON parse failed: %s\n", error.c_str());
        request->send(400, "text/plain", "Invalid JSON format: " + String(error.c_str()));
        return;
      }

      // Update configuration variables
      // ... (Existing network and timing updates) ...

      if (doc.containsKey("wifiSSID")) strncpy(appConfig.wifiSSID, doc["wifiSSID"], sizeof(appConfig.wifiSSID));
      if (doc.containsKey("wifiPass") && doc["wifiPass"].as<String>().length() > 0)
        strncpy(appConfig.wifiPass, doc["wifiPass"], sizeof(appConfig.wifiPass));

      if (doc.containsKey("mqttServer")) strncpy(appConfig.mqttServer, doc["mqttServer"], sizeof(appConfig.mqttServer));
      if (doc.containsKey("mqttPort")) appConfig.mqttPort = doc["mqttPort"].as<uint16_t>();
      if (doc.containsKey("mqttUser")) strncpy(appConfig.mqttUser, doc["mqttUser"], sizeof(appConfig.mqttUser));
      if (doc.containsKey("mqttPass") && doc["mqttPass"].as<String>().length() > 0)
        strncpy(appConfig.mqttPass, doc["mqttPass"], sizeof(appConfig.mqttPass));
      if (doc.containsKey("mqttTopic")) strncpy(appConfig.mqttTopic, doc["mqttTopic"], sizeof(appConfig.mqttTopic));

      if (doc.containsKey("sendInterval")) appConfig.sendInterval = doc["sendInterval"].as<uint32_t>();
      if (doc.containsKey("ntpServer")) strncpy(appConfig.ntpServer, doc["ntpServer"], sizeof(appConfig.ntpServer));

      // --- ADDED: SENSOR PINS SAVING ---
      if (doc.containsKey("dustLEDPin")) appConfig.dustLEDPin = doc["dustLEDPin"].as<uint8_t>();
      if (doc.containsKey("dustADCPin")) appConfig.dustADCPin = doc["dustADCPin"].as<uint8_t>();
      if (doc.containsKey("mqADCPin")) appConfig.mqADCPin = doc["mqADCPin"].as<uint8_t>();
      
      // --- ADDED: MQ Calibration Parameters (parsing floats) ---
      if (doc.containsKey("mq_rl_kohm")) appConfig.mq_rl_kohm = doc["mq_rl_kohm"].as<float>();
      if (doc.containsKey("mq_r0_ratio_clean")) appConfig.mq_r0_ratio_clean = doc["mq_r0_ratio_clean"].as<float>();
      if (doc.containsKey("tvoc_a_curve")) appConfig.tvoc_a_curve = doc["tvoc_a_curve"].as<float>();
      if (doc.containsKey("tvoc_b_curve")) appConfig.tvoc_b_curve = doc["tvoc_b_curve"].as<float>();

      // Save and reboot to apply changes
      saveConfig();
      request->send(200, "text/plain", "Settings saved. Rebooting now to apply changes.");
      Serial.println("Web: Configuration updated. Rebooting...");
      delay(1000);
      ESP.restart();
    });


  // Control commands
  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
    handleReboot(request);
  });

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
    handleReset(request);
  });


  server.begin();
  // Print server start information, using either AP or STA IP address
  Serial.printf("Web: Server started on http://%s/ (Dashboard) and http://%s/settings (Config)\n",
                isWifiConnected ? WiFi.localIP().toString().c_str() : WiFi.softAPIP().toString().c_str(),
                isWifiConnected ? WiFi.localIP().toString().c_str() : WiFi.softAPIP().toString().c_str());
}