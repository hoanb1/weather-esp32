#pragma once
#include <Arduino.h>

String getDashboardPageHTML() {
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
nav{padding:10px;background:#2c3e50;color:white;text-align:center;display:flex;}
nav h1{margin:0;}
nav a{color:white;text-decoration:none;margin-left:10px;}
#logArea{height:450px;overflow:auto;background:#000;color:#0f0;padding:10px;font-family:monospace;margin:20px;}
</style>
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<script src="https://cdn.jsdelivr.net/npm/luxon@3/build/global/luxon.min.js"></script>
<script src="https://cdn.jsdelivr.net/npm/chartjs-adapter-luxon@1"></script>
</head>
<body>
<nav><h1>ESP32 Dashboard</h1><a href="/settings">Settings</a> | <a href="/ota">OTA</a></nav>
<div class="grid">
<div class="card"><h4>Temperature/Humidity</h4><div id="tempHumVal">--</div><canvas id="chartTempHum"></canvas></div>
<div class="card"><h4>Pressure</h4><div id="presVal">--</div><canvas id="chartPres"></canvas></div>
<div class="card"><h4>Dust (PM)</h4><div id="dustVal">--</div><canvas id="chartDust"></canvas></div>
<div class="card"><h4>MQ (ADC)</h4><div id="mqVal">--</div><canvas id="chartMQ"></canvas></div>
<div class="card"><h4>AQI</h4><div id="aqiVal">--</div><canvas id="chartAQI"></canvas></div>
</div>
0.
<div id="logArea"></div>
<script src="dashboard.js"></script>
</body>
</html>
)rawliteral";

    return html;
}
