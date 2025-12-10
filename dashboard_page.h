//dashboard_page.h
//dashboard_page.h
#pragma once
#include <Arduino.h>

String getDashboardPageHTML() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>ESP32 Monitoring Dashboard</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
/* --- Global Styles --- */
body {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    background: #e9eef2;
    margin: 0;
    padding: 0;
    color: #333;
}

/* --- Navigation Bar --- */
.navbar {
    background: #2c3e50;
    color: white;
    padding: 10px 10px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
}
.navbar-title {
    font-size: 1.1em;
    margin: 0;
}
.nav-links {
    display: flex;
    gap: 10px;
}
.nav-links a {
    color: white;
    text-decoration: none;
    font-weight: 500;
    padding: 5px 10px;
    border-radius: 5px;
    transition: background 0.3s;
}
.nav-links a:hover {
    background: #34495e;
}

/* --- Main Layout --- */
.container {
    padding: 20px;
}
.dashboard-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
    gap: 20px;
}

/* --- Card Styles (for sensor data/charts) --- */
.card {
    background: white;
    padding: 20px;
    border-radius: 12px;
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.08);
    transition: transform 0.3s;
}
.card:hover {
    transform: translateY(-3px);
    box-shadow: 0 6px 16px rgba(0, 0, 0, 0.1);
}
.card-header {
    font-size: 1em;
    margin-top: 0;
    margin-bottom: 10px;
    color: #2c3e50;
    border-bottom: 2px solid #ecf0f1;
    padding-bottom: 5px;
}
.current-value {
    font-size: 1em;
    font-weight: bold;
    color: #3498db;
    margin-bottom: 15px;
    text-align: center;
}
canvas {
    width: 100% !important;
    height: 180px !important; /* Slightly reduced height for better card fit */
}

/* --- Log Area --- */
#logArea {
    height: 300px;
    overflow: auto;
    background: #1c1c1c; /* Darker background for logs */
    color: #00ff66; /* Bright green text */
    padding: 15px;
    font-family: 'Consolas', 'Courier New', monospace;
    margin-top: 20px;
    border-radius: 8px;
    white-space: pre-wrap;
    box-shadow: inset 0 0 8px rgba(0, 0, 0, 0.2);
}
</style>

<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<script src="https://cdn.jsdelivr.net/npm/luxon@3/build/global/luxon.min.js"></script>
<script src="https://cdn.jsdelivr.net/npm/chartjs-adapter-luxon@1"></script>

</head>
<body>

<header class="navbar">
  <h1 class="navbar-title">Dashboard</h1>
  <div class="nav-links">
    <a href="/settings">Settings</a>
    <a href="/ota">OTA</a>
    <a href="/calibrate">Calibrate</a>
  </div>
</header>

<div class="container">

<section class="dashboard-grid">

<div class="card">
  <h4 class="card-header">Temperature / Humidity</h4>
  <div class="current-value" id="tempHumVal">--</div>
  <canvas id="chartTempHum"></canvas>
</div>

<div class="card">
  <h4 class="card-header">Pressure</h4>
  <div class="current-value" id="presVal">--</div>
  <canvas id="chartPres"></canvas>
</div>

<div class="card">
  <h4 class="card-header">PM2.5 Concentration</h4>
  <div class="current-value" id="dustVal">--</div>
  <canvas id="chartDust"></canvas>
</div>

<div class="card">
  <h4 class="card-header">Air Quality Index (AQI)</h4>
  <div class="current-value" id="aqiVal">--</div>
  <canvas id="chartAQI"></canvas>
</div>

<div class="card">
  <h4 class="card-header">MQ135 Gas Index</h4>
  <div class="current-value" id="mqVal">--</div>
  <canvas id="chartMQ"></canvas>
</div>

<div class="card">
    <h4 class="card-header">System Info</h4>
    <div class="current-value" id="sysInfoVal">Uptime: --</div>
    <p style="text-align: center; color: #7f8c8d;">Status: <span id="statusIndicator">Connecting...</span></p>
</div>

</section>

<h3 style="margin-top: 40px; color: #2c3e50;">Device Logs</h3>
<pre id="logArea"></pre>

</div>

<script src="dashboard.js"></script>
</body>
</html>
)rawliteral";

    return html;
}
