#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <cstdint>
#include <cstring>

#include "config.h"  // appConfig
#include "data.h"    // ws, server, onWsEvent, isWifiConnected

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ======== Reboot / Reset handlers ========
void handleReboot(AsyncWebServerRequest *request) {
  request->send(200, "text/html", "<html><body>Rebooting...</body></html>");
  addLog("Manual reboot requested. Restarting...");
  delay(1000);
  ESP.restart();
}

void handleReset(AsyncWebServerRequest *request) {
  request->send(200, "text/html", "<html><body>Factory reset. Rebooting...</body></html>");
  addLog("Factory reset requested. Erasing NVS...");
  resetConfig();
  delay(1000);
  ESP.restart();
}

// ======== Settings page ========
String getSettingsPage() {
  StaticJsonDocument<512> doc;
  doc["deviceId"] = appConfig.deviceId;
  doc["latitude"] = appConfig.latitude;
  doc["longitude"] = appConfig.longitude;
  doc["wifiSSID"] = appConfig.wifiSSID;
  doc["mqttServer"] = appConfig.mqttServer;
  doc["mqttPort"] = appConfig.mqttPort;
  doc["mqttUser"] = appConfig.mqttUser;
  doc["mqttTopic"] = appConfig.mqttTopic;
  doc["sendInterval"] = appConfig.sendInterval;
  doc["ntpServer"] = appConfig.ntpServer;
  doc["dustLEDPin"] = appConfig.dustLEDPin;
  doc["dustADCPin"] = appConfig.dustADCPin;
  doc["mqADCPin"] = appConfig.mqADCPin;
  doc["mq_rl_kohm"] = appConfig.mq_rl_kohm;
  doc["mq_r0_ratio_clean"] = appConfig.mq_r0_ratio_clean;
  doc["tvoc_a_curve"] = appConfig.tvoc_a_curve;
  doc["tvoc_b_curve"] = appConfig.tvoc_b_curve;

  String jsonConfig;
  serializeJson(doc, jsonConfig);

  String statusColor = isWifiConnected ? "green" : "red";
  String statusIP = isWifiConnected ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  String statusMsg = "<span style='color:" + statusColor + ";'>" + (isWifiConnected ? "Connected: " : "Hotspot Mode: ") + statusIP + "</span>";

  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
body{font-family:Arial;background:#f4f7f6;padding:20px;color:#333;}
.container{max-width:500px;margin:0 auto;background:#fff;padding:20px;border-radius:10px;box-shadow:0 4px 10px rgba(0,0,0,0.1);}
h2{color:#2c3e50;border-bottom:2px solid #3498db;padding-bottom:10px;}
label{display:block;margin-top:10px;font-weight:bold;}
input[type=text],input[type=password],input[type=number]{width:calc(100% - 20px);padding:10px;margin-top:5px;border:1px solid #ccc;border-radius:5px;}
button{background:#2ecc71;color:white;padding:10px 15px;border:none;border-radius:5px;cursor:pointer;margin-top:20px;font-size:16px;}
button:hover{background:#27ae60;}
.button-group button{margin-right:10px;}
.warning-btn{background:#e74c3c;}
.warning-btn:hover{background:#c0392b;}
.status{margin-bottom:20px;font-weight:bold;padding:10px;border-radius:5px;background:#ecf0f1;}
#logArea{height:200px;overflow:auto;background:#000;color:#0f0;padding:10px;font-family:monospace;margin-top:20px;}
</style>
</head>
<body>
<div class="container">
<h2>System Configuration</h2>
<div class="status">)rawliteral"
                + statusMsg + R"rawliteral(</div>
<form id="configForm">
<label for="deviceId">Device ID:</label><input type="text" id="deviceId" name="deviceId">
<label for="latitude">Latitude:</label><input type="number" step="0.000001" id="latitude" name="latitude">
<label for="longitude">Longitude:</label><input type="number" step="0.000001" id="longitude" name="longitude">
<label for="wifiSSID">WiFi SSID:</label><input type="text" id="wifiSSID" name="wifiSSID">
<label for="wifiPass">WiFi Password:</label><input type="password" id="wifiPass" name="wifiPass">
<label for="mqttServer">MQTT Server:</label><input type="text" id="mqttServer" name="mqttServer">
<label for="mqttPort">MQTT Port:</label><input type="number" id="mqttPort" name="mqttPort">
<label for="mqttUser">MQTT User:</label><input type="text" id="mqttUser" name="mqttUser">
<label for="mqttPass">MQTT Pass:</label><input type="password" id="mqttPass" name="mqttPass">
<label for="mqttTopic">MQTT Topic:</label><input type="text" id="mqttTopic" name="mqttTopic">
<label for="sendInterval">Send Interval (ms):</label><input type="number" id="sendInterval" name="sendInterval">
<label for="ntpServer">NTP Server:</label><input type="text" id="ntpServer" name="ntpServer">
<label for="dustLEDPin">Dust LED Pin:</label><input type="number" id="dustLEDPin" name="dustLEDPin">
<label for="dustADCPin">Dust ADC Pin:</label><input type="number" id="dustADCPin" name="dustADCPin">
<label for="mqADCPin">MQ ADC Pin:</label><input type="number" id="mqADCPin" name="mqADCPin">
<label for="mq_rl_kohm">MQ RL (kOhm):</label><input type="number" step="0.01" id="mq_rl_kohm" name="mq_rl_kohm">
<label for="mq_r0_ratio_clean">MQ R0 ratio:</label><input type="number" step="0.001" id="mq_r0_ratio_clean" name="mq_r0_ratio_clean">
<label for="tvoc_a_curve">TVOC Curve A:</label><input type="number" step="0.001" id="tvoc_a_curve" name="tvoc_a_curve">
<label for="tvoc_b_curve">TVOC Curve B:</label><input type="number" step="0.001" id="tvoc_b_curve" name="tvoc_b_curve">
<button type="submit">Save & Reboot</button>
</form>
<div class="button-group">
<button onclick="window.location.href='/'">Dashboard</button>
<button class="warning-btn" onclick="if(confirm('Reboot?')) window.location.href='/reboot'">Reboot</button>
<button class="warning-btn" onclick="if(confirm('Factory reset?')) window.location.href='/reset'">Factory Reset</button>
</div>


<script>
const configData = JSON.parse(')rawliteral"
                + jsonConfig + R"rawliteral(');

for(const key in configData) {
    if(document.getElementById(key)) document.getElementById(key).value=configData[key];
}

document.getElementById('configForm').onsubmit = function(e){
    e.preventDefault();
    const data = {};
    new FormData(this).forEach((v,k)=>{data[k]=v;});
    fetch('/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(data)})
    .then(r=>r.text()).then(t=>alert(t));
};


</script>
</div>
</body>
</html>
)rawliteral";

  return html;
}

// ======== Dashboard page (Updated: Dual Axis Temp/Hum, 3-Column Grid, Thinnest Lines) ========
String getDashboardPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>ESP32 Dashboard</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body{font-family:Arial;background:#f2f2f2;margin:0;padding:0;}
/* Grid: 3 columns on large screens, responsive down to 1 column on mobile */
.grid{
    display:grid;
    grid-template-columns:repeat(auto-fit,minmax(300px,1fr));
    grid-gap:15px;
    padding:15px;
}
.card{background:white;padding:10px;border-radius:10px;box-shadow:0 4px 8px rgba(0,0,0,0.1);}
/* Increased canvas height */
canvas{width:100% !important;height:220px !important;} 
nav{padding:10px;background:#2c3e50;color:white;text-align:center;display:flex;justify-content:space-between;}
nav h1{margin:0;}
nav a{color:white;text-decoration:none;margin-left:10px;}
#logArea{height:450px;overflow:auto;background:#000;color:#0f0;padding:10px;font-family:monospace;margin:20px;}
</style>
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<script src="https://cdn.jsdelivr.net/npm/luxon@3/build/global/luxon.min.js"></script>
<script src="https://cdn.jsdelivr.net/npm/chartjs-adapter-luxon@1"></script>
</head>
<body>
<nav><h1>ESP32 Dashboard</h1><a href="/settings">Settings</a></nav>
<div class="grid">
<div class="card"><h4>Temperature/Humidity</h4><div id="tempHumVal">--</div><canvas id="chartTempHum"></canvas></div>
<div class="card"><h4>Pressure</h4><div id="presVal">--</div><canvas id="chartPres"></canvas></div>
<div class="card"><h4>Dust (PM)</h4><div id="dustVal">--</div><canvas id="chartDust"></canvas></div>
<div class="card"><h4>MQ (ADC)</h4><div id="mqVal">--</div><canvas id="chartMQ"></canvas></div>
<div class="card"><h4>AQI</h4><div id="aqiVal">--</div><canvas id="chartAQI"></canvas></div>
</div>
<h3>Device Logs</h3>
<div id="logArea"></div>
<script>
// Separated tempHumData into tempData and humData
let tempData=[], humData=[], presData=[], dustData=[], mqData=[], aqiData=[];
const maxPoints=120;

function addData(arr,ts,v){
    // ts is in microseconds, divide by 1000 to get milliseconds
    const x_val = ts / 1000; 
    arr.push({x: x_val, y: v});
    if(arr.length>maxPoints) arr.shift();
}

// Basic chart function (1 dataset)
function createChart(ctx, data, color, chartLabel){
    return new Chart(ctx,{
        type:'line',
        data:{
            datasets:[{
                label: chartLabel, 
                data:data,
                borderColor:color,
                // Reduced line width to 1
                borderWidth: 0.5, 
                fill:false,
                tension:0.4,
                pointRadius:0
            }]
        },
        options:{
            animation:{duration:0},
            scales:{
                x:{
                    type:'time',
                    time:{
                        unit:'second'
                    },
                },
                y:{beginAtZero:true}
            }
        }
    });
}

// Specialized function for Temp/Humidity Chart (2 Datasets, 2 Y-Axes)
function createTempHumChart(ctx, tempData, humData){
    return new Chart(ctx, {
        type: 'line',
        data: {
            datasets: [{
                label: 'Temperature (°C)',
                data: tempData,
                borderColor: '#e74c3c', // Red for Temperature
                backgroundColor: '#e74c3c',
                yAxisID: 'yTemp', // Y-Axis for Temperature
                // Reduced line width to 1
                borderWidth: 1, 
                fill: false,
                tension: 0.4,
                pointRadius: 0
            }, {
                label: 'Humidity (%)',
                data: humData,
                borderColor: '#3498db', // Blue for Humidity
                backgroundColor: '#3498db',
                yAxisID: 'yHum', // Y-Axis for Humidity
                // Reduced line width to 1
                borderWidth: 1, 
                fill: false,
                tension: 0.4,
                pointRadius: 0
            }]
        },
        options: {
            animation: {duration: 0},
            scales: {
                x: { type: 'time', time: { unit: 'second' } },
                yTemp: {
                    type: 'linear',
                    position: 'left', // Left Y-axis for Temperature
                    title: { display: true, text: 'Temperature (°C)' },
                    beginAtZero: false 
                },
                yHum: {
                    type: 'linear',
                    position: 'right', // Right Y-axis for Humidity
                    title: { display: true, text: 'Humidity (%)' },
                    beginAtZero: true,
                    // Limits humidity range to 0-100%
                    min: 0, 
                    max: 100,
                    grid: { drawOnChartArea: false } // Do not draw grid for this axis
                }
            }
        }
    });
}

// Initialize specialized Temp/Hum Chart
const chartTempHum = createTempHumChart(document.getElementById('chartTempHum').getContext('2d'), tempData, humData);

// Initialize remaining charts (1 dataset each)
const chartPres=createChart(document.getElementById('chartPres').getContext('2d'),presData,'#3498db', 'Pressure (hPa)');
const chartDust=createChart(document.getElementById('chartDust').getContext('2d'),dustData,'#964B00', 'Dust PM (µg/m³)');
const chartMQ=createChart(document.getElementById('chartMQ').getContext('2d'),mqData,'#f39c12', 'MQ Sensor (ADC)');
const chartAQI=createChart(document.getElementById('chartAQI').getContext('2d'),aqiData,'#8e44ad', 'AQI');

let ws = new WebSocket('ws://'+location.hostname+'/ws');
ws.onmessage = e=>{
    const d=JSON.parse(e.data);
    const ts_us = d.ts; 
    
    if(d.t!==undefined && d.h!==undefined){
        document.getElementById('tempHumVal').innerText=d.t.toFixed(1)+'°C / '+d.h.toFixed(1)+'%';
        
        // Separate updates for Temperature and Humidity
        addData(tempData,ts_us,d.t); 
        addData(humData,ts_us,d.h); 

        chartTempHum.update();
    }
    if(d.p!==undefined){
        document.getElementById('presVal').innerText=d.p.toFixed(1) + ' hPa';
        addData(presData,ts_us,d.p);
        chartPres.update();
    }
    if(d.pm!==undefined){
        document.getElementById('dustVal').innerText=d.pm.toFixed(1) + ' µg/m³';
        addData(dustData,ts_us,d.pm);
        chartDust.update();
    }
    if(d.mq!==undefined){
        document.getElementById('mqVal').innerText=d.mq.toFixed(0); 
        addData(mqData,ts_us,d.mq);
        chartMQ.update();
    }
    if(d.aqi!==undefined){
        document.getElementById('aqiVal').innerText=d.aqi;
        addData(aqiData,ts_us,d.aqi);
        chartAQI.update();
    }
    
    if(d.msg!==undefined){
        let logDiv=document.getElementById('logArea');
        logDiv.innerText+=d.msg+"\n";
        logDiv.scrollTop=logDiv.scrollHeight;
    }
};
</script>
</body>
</html>
)rawliteral";

  return html;
}

// ======== Send data to WebSocket ========
void sendWSData(const char *json) {
  ws.textAll(json);
}

// ======== Setup server ========
void setupWebServer() {
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // Dashboard
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = getDashboardPage();
    request->send(200, "text/html; charset=utf-8", html);
  });

  // Settings
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html; charset=utf-8", getSettingsPage());
  });

  // Save config
  server.on(
    "/save", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, (const char *)data, len);
      if (error) {
        request->send(400, "text/plain", "Invalid JSON");
        return;
      }

      if (doc.containsKey("deviceId")) strncpy(appConfig.deviceId, doc["deviceId"], sizeof(appConfig.deviceId));
      if (doc.containsKey("latitude")) appConfig.latitude = doc["latitude"].as<float>();
      if (doc.containsKey("longitude")) appConfig.longitude = doc["longitude"].as<float>();
      if (doc.containsKey("wifiSSID")) strncpy(appConfig.wifiSSID, doc["wifiSSID"], sizeof(appConfig.wifiSSID));
      if (doc.containsKey("wifiPass")) strncpy(appConfig.wifiPass, doc["wifiPass"], sizeof(appConfig.wifiPass));
      if (doc.containsKey("mqttServer")) strncpy(appConfig.mqttServer, doc["mqttServer"], sizeof(appConfig.mqttServer));
      if (doc.containsKey("mqttPort")) appConfig.mqttPort = doc["mqttPort"].as<uint16_t>();
      if (doc.containsKey("mqttUser")) strncpy(appConfig.mqttUser, doc["mqttUser"], sizeof(appConfig.mqttUser));
      if (doc.containsKey("mqttPass")) strncpy(appConfig.mqttPass, doc["mqttPass"], sizeof(appConfig.mqttPass));
      if (doc.containsKey("mqttTopic")) strncpy(appConfig.mqttTopic, doc["mqttTopic"], sizeof(appConfig.mqttTopic));
      if (doc.containsKey("sendInterval")) appConfig.sendInterval = doc["sendInterval"].as<uint32_t>();
      if (doc.containsKey("ntpServer")) strncpy(appConfig.ntpServer, doc["ntpServer"], sizeof(appConfig.ntpServer));
      saveConfig();
      addLog("Configuration updated. Rebooting...");
      request->send(200, "text/plain", "Settings saved. Rebooting...");
      delay(1000);
      ESP.restart();
    });

  server.on("/reboot", HTTP_GET, handleReboot);
  server.on("/reset", HTTP_GET, handleReset);

  server.begin();
  String msg = "Web server started on http://";
  msg += isWifiConnected ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  addLog(msg.c_str());
}