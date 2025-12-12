#pragma once
#include <Arduino.h>

String getDashboardPageHTML() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>Flat Modern ESP32 Dashboard</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link href="https://fonts.googleapis.com/css2?family=Poppins:wght@300;400;500;600;700&display=swap" rel="stylesheet">
<script src="https://code.highcharts.com/highcharts.js"></script>
<script src="https://code.highcharts.com/highcharts-more.js"></script>
<script src="https://code.highcharts.com/modules/solid-gauge.js"></script>
<script src="https://code.highcharts.com/modules/exporting.js"></script>
<script src="https://code.highcharts.com/modules/export-data.js"></script>
<script src="https://code.highcharts.com/modules/accessibility.js"></script>

<style>
/* --- Flat & Modern UI Variables --- */
:root {
    --color-primary: #007bff;      /* Blue */
    --color-secondary: #34495e;    /* Dark Slate/Navy */
    --color-background: #f0f2f5;   /* Light Grey Background */
    --color-surface: #ffffff;      /* White/Transparent Look */
    --color-text-dark: #2c3e50;    /* Text */
    --color-text-muted: #95a5a6;
    --color-success: #2ecc71;      /* Green */
    --color-warning: #f39c12;      /* Yellow/Orange */
    --color-danger: #e74c3c;       /* Red */
    --color-line-separator: #e0e3e6; /* Very light grey for separation */
    --gauge-size: 140px;
}
body {
    font-family: 'Poppins', sans-serif;
    background: var(--color-background);
    margin: 0;
    padding: 0;
    color: var(--color-text-dark);
}

/* --- Navbar (Minimal change - keep structure) --- */
.navbar {
    background: var(--color-surface); /* Changed to white surface */
    color: var(--color-text-dark);   /* Text dark */
    padding: 15px 30px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    box-shadow: 0 1px 0 rgba(0, 0, 0, 0.05); /* Thin separator line */
}
.navbar-title {
    font-size: 1.2em;
    margin: 0;
    font-weight: 600;
}
.nav-links {
    display: flex;
    font-size:0.8em;
}
.nav-links a {
    color: var(--color-text-dark); /* Changed to dark text */
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
    background: var(--color-background); /* Hover effect is background color */
    transform: none; /* Removed transform for flatter feel */
}

.main-content {
    padding: 30px;
    max-width: 1500px;
    margin: 0 auto;
}

/* --- Control Bar (Flat Redesign) --- */
.control-bar {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 25px;
    padding: 15px 0; /* Removed horizontal padding/background for flat look */
    background: transparent; /* No background */
    border-radius: 0;
    box-shadow: none; /* No shadow */
    border-bottom: 1px solid var(--color-line-separator); /* Thin separator line */
    border-top: 1px solid var(--color-line-separator);
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

/* --- Tile Grid (No Panel/Card Redesign) --- */
.data-grid {
    display: grid;
    grid-template-columns: repeat(3, 1fr); 
    gap: 10px; /* Reduced gap for tighter, flatter layout */
}
.data-tile {
    background: #ffffff;
    padding: 20px 0; /* Adjusted padding */
    border-radius: 0;
    border: none; /* NO BORDER */
    box-shadow: none; /* NO SHADOW */
    transition: all 0.3s;
    display: flex; 
    flex-direction: column;
    /* Add subtle separator line below each tile/column for flat separation */
    border-bottom: 1px solid var(--color-line-separator);
}
/* Ensure the last row doesn't have the bottom separator or if easier, handle it via parent container */
/* A grid-based approach often means styling individual items */
/* Example: Targetting elements that are not in the last row (requires knowing the grid setup)
.data-grid:last-child .data-tile { border-bottom: none; }
For simplicity, we leave the separator on all tiles or use a different separator approach.
Here, we keep it simple: separated by background and line.
*/

.data-tile:hover {
    box-shadow: none; /* NO HOVER SHADOW */
    border-color: none;
    background: #ffffff;
}
.tile-header {
    font-size: 1.1em;
    margin-bottom: 15px;
    font-weight: 600;
    color: var(--color-text-dark);
    text-align: center;
    padding-bottom: 5px;
    border-bottom: 1px solid var(--color-line-separator); /* Use thinner, flat line */
}

/* --- Media Query for smaller screens (Below 900px) --- */
@media (max-width: 900px) {
    .data-grid {
        /* Revert to auto-fit or 2 columns on narrower screens */
        grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
    }
}
/* --- Highcharts Gauge Container CSS (Keep size) --- */
.gauge-container {
    height: 150px;
    width: 100%;
    margin: 0 auto; 
}

/* --- Highcharts Line Chart Containers (Keep structure) --- */
.chart-container {
    min-height: 180px;
    flex-grow: 1;
    margin-top: 5px; 
}

/* --- Log Panel (Flat Redesign) --- */
.log-panel {
    grid-column: 1 / -1; 
    margin-top: 25px;
    background: transparent; /* NO CARD BACKGROUND */
    border-radius: 0;
    padding: 0; /* No Padding */
    box-shadow: none; /* NO SHADOW */
}
.log-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 10px;
    padding-bottom: 10px;
    border-bottom: 1px solid var(--color-line-separator); /* Separator line */
}
.log-toggle {
    background: var(--color-primary); /* Use primary color */
    color: white;
    border: none;
    padding: 8px 15px;
    border-radius: 5px;
    cursor: pointer;
    font-weight: 500;
    transition: background 0.3s;
}
.log-toggle:hover {
    background: #0056b3; /* Darker primary on hover */
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
    border-top: 1px solid var(--color-line-separator); /* Separator line */
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
        <h4 class="tile-header" style="margin-bottom:0; border-bottom: none; padding-bottom: 0;">Device Logs</h4>
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