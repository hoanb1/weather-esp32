#pragma once
#include <Arduino.h>

String getDashboardPageHTML() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>Modern ESP32 Dashboard</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link href="https://fonts.googleapis.com/css2?family=Poppins:wght@300;400;500;600;700&display=swap" rel="stylesheet">
<script src="https://code.highcharts.com/highcharts.js"></script>
<script src="https://code.highcharts.com/highcharts-more.js"></script>
<script src="https://code.highcharts.com/modules/solid-gauge.js"></script> <script src="https://code.highcharts.com/modules/exporting.js"></script>
<script src="https://code.highcharts.com/modules/export-data.js"></script>
<script src="https://code.highcharts.com/modules/accessibility.js"></script>

<style>
/* --- Flat & Modern UI Variables --- */
:root {
    --color-primary: #007bff;      /* Blue */
    --color-secondary: #34495e;    /* Dark Slate/Navy */
    --color-background: #f0f2f5;   /* Light Grey Background */
    --color-surface: white;        /* Tile Background */
    --color-text-dark: #2c3e50;    /* Text */
    --color-text-muted: #95a5a6;
    --color-success: #2ecc71;      /* Green */
    --color-warning: #f39c12;      /* Yellow/Orange */
    --color-danger: #e74c3c;       /* Red */
    --gauge-size: 140px; 
}
body {
    font-family: 'Poppins', sans-serif;
    background: var(--color-background);
    margin: 0;
    padding: 0;
    color: var(--color-text-dark);
}
/* --- Navbar (No change) --- */
.navbar {
    background: var(--color-secondary);
    color: white;
    padding: 15px 30px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
}
.navbar-title {
    font-size: 1.2em;
    margin: 0;
    font-weight: 600;
}
.nav-links {
    display: flex;
    gap: 10px;
}
.nav-links a {
    color: white;
    text-decoration: none;
    font-weight: 500;
    padding: 8px 6px;
    border-radius: 6px;
    transition: background 0.3s, transform 0.1s;
    display: flex;
    align-items: center;
    gap: 8px;
}
.nav-links a:hover {
    background: rgba(255, 255, 255, 0.1);
    transform: translateY(-1px);
}

.main-content {
    padding: 30px;
    max-width: 1500px;
    margin: 0 auto;
}

/* --- Control Bar and System Info (No change) --- */
.control-bar {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 25px;
    padding: 15px;
    background: var(--color-surface);
    border-radius: 8px;
    box-shadow: 0 2px 4px rgba(0,0,0,0.05);
}
.sys-info {
    font-size: 0.9em;
    font-weight: 500;
    color: var(--color-text-dark);
}
.sys-info span {
    margin-right: 15px;
}
#statusIndicator {
    font-weight: 600;
    padding: 4px 10px;
    border-radius: 4px;
    transition: background-color 0.5s;
    color: white; 
}

/* --- Tile Grid (No change) --- */
.data-grid {
    display: grid;
    /* Force 3 equal columns on wide screens */
    grid-template-columns: repeat(3, 1fr); 
    gap: 20px;
}
.data-tile {
    background: var(--color-surface);
    padding: 20px;
    border-radius: 8px;
    border: 1px solid #ecf0f1; 
    box-shadow: 0 4px 12px rgba(0,0,0,0.05); 
    transition: all 0.3s;
    display: flex; 
    flex-direction: column;
}
.data-tile:hover {
    box-shadow: 0 6px 15px rgba(0, 123, 255, 0.15); 
    border-color: var(--color-primary);
}
.tile-header {
    font-size: 1.2em;
    margin-bottom: 15px;
    font-weight: 600;
    color: var(--color-text-dark);
    text-align: center;
    padding-bottom: 5px;
    border-bottom: 2px solid #ecf0f1;
}

/* --- Media Query for smaller screens (Below 900px) --- */
@media (max-width: 900px) {
    .data-grid {
        /* Revert to auto-fit or 2 columns on narrower screens */
        grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
    }
}
/* --- Highcharts Gauge Container CSS --- */
.gauge-container {
    height: 150px; /* Increased height to fit Highcharts gauge better */
    width: 100%;
    margin: 0 auto; 
}

/* --- Highcharts Line Chart Containers (No change) --- */
.chart-container {
    min-height: 180px;
    flex-grow: 1;
    margin-top: 5px; 
}

/* --- Log Panel (No change) --- */
.log-panel {
    grid-column: 1 / -1; 
    margin-top: 20px;
    background: var(--color-surface);
    border-radius: 8px;
    padding: 20px;
    box-shadow: 0 4px 12px rgba(0,0,0,0.05);
}
.log-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 10px;
}
.log-toggle {
    background: var(--color-secondary);
    color: white;
    border: none;
    padding: 8px 15px;
    border-radius: 5px;
    cursor: pointer;
    font-weight: 500;
    transition: background 0.3s;
}
.log-toggle:hover {
    background: #5a6a7c;
}

#logAreaWrapper {
    max-height: 0;
    overflow: hidden;
    transition: max-height 0.5s ease-out, opacity 0.5s ease-out;
    opacity: 0;
}
#logAreaWrapper.expanded {
    max-height: 500px; 
    opacity: 1;
    margin-top: 15px;
    border-top: 1px solid #ecf0f1;
    padding-top: 10px;
}
#logArea {
    height: 250px; 
    overflow-y: auto;
    background: #1e2a36; 
    color: #3498db; 
    padding: 10px;
    font-family: 'Consolas', 'Courier New', monospace;
    border-radius: 4px;
    white-space: pre-wrap;
    margin: 0;
}
</style>
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

<div class="main-content">

<div class="control-bar">
    <div class="sys-info">
        <span id="sysInfoVal">Uptime: -- | RSSI: -- dBm</span>
        <span>Status: <span id="statusIndicator">Connecting...</span></span>
    </div>

    <div class="time-control">
        <label for="timeRangeSelect">Time Range:</label>
        <select id="timeRangeSelect">
            <option value="60">1 min</option>
            <option value="300">5 min</option>
            <option value="900">15 min</option>
            <option value="3600">1 hr</option>
            <option value="10800">3 hr</option>
            <option value="21600">6 hr</option> 
            <option value="43200">12 hr</option> 
            <option value="86400">1 day</option>
        </select>
    </div>
</div>

<section class="data-grid">

<div class="data-tile">
  <h4 class="tile-header">Temperature (°C)</h4>
  <div id="gaugeTemp" class="gauge-container"></div>
  <div id="chartTemp" class="chart-container"></div>
</div>

<div class="data-tile">
  <h4 class="tile-header">Humidity (%)</h4>
  <div id="gaugeHum" class="gauge-container"></div>
  <div id="chartHum" class="chart-container"></div>
</div>

<div class="data-tile">
  <h4 class="tile-header">Pressure (hPa)</h4>
  <div id="gaugePres" class="gauge-container"></div>
  <div id="chartPres" class="chart-container"></div>
</div>

<div class="data-tile">
  <h4 class="tile-header">PM2.5 (µg/m³)</h4>
  <div id="gaugeDust" class="gauge-container"></div>
  <div id="chartDust" class="chart-container"></div>
</div>

<div class="data-tile">
  <h4 class="tile-header">Air Quality Index (AQI)</h4>
  <div id="gaugeAQI" class="gauge-container"></div>
  <div id="chartAQI" class="chart-container"></div>
</div>

<div class="data-tile">
  <h4 class="tile-header">MQ Gas Index</h4>
  <div id="gaugeMQ" class="gauge-container"></div>
  <div id="chartMQ" class="chart-container"></div>
</div>

</section>

<div class="log-panel">
    <div class="log-header">
        <h4 class="tile-header" style="margin-bottom:0; border-bottom: none;">Device Logs</h4>
        <button id="logToggleBtn" class="log-toggle">Show Logs</button>
    </div>
    <div id="logAreaWrapper">
        <pre id="logArea"></pre>
    </div>
</div>


</div>

<script src="dashboard.js"></script>
</body>
</html>
)rawliteral";

    return html;
}