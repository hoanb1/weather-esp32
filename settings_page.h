//setting_page.h
#pragma once
#include <Arduino.h>


String getSettingsPageHTML(const String &statusMsg, const String &jsonConfig) {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>System Settings</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
body{font-family:'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;background:#e9eef2;padding:20px;color:#333;font-size: 0.8rem;}
.container{max-width:600px;margin:0 auto;background:#fff;padding:25px;border-radius:12px;box-shadow:0 6px 16px rgba(0,0,0,0.1);}
h2{color:#2c3e50;border-bottom:2px solid #3498db;padding-bottom:10px;margin-top:0;}
h3{color:#34495e;margin-top:20px;padding-bottom:5px;border-bottom:1px dashed #ccc;}
.form-row{display:flex;justify-content:space-between;align-items:center;margin-bottom:12px;}
.form-row label{width:45%;color:#555;}
.form-row input[type=text],.form-row input[type=password],.form-row input[type=number]{width:50%;padding:10px;border:1px solid #ddd;border-radius:6px;box-sizing:border-box;}
.btn-primary{background:#3498db;color:white;padding:12px 20px;border:none;border-radius:6px;cursor:pointer;font-size:16px;transition:background 0.3s;}
.btn-primary:hover{background:#2980b9;}
.btn-group{display:flex;justify-content:flex-end;}
.btn-group button{ margin-left:10px;
    padding:12px 20px;
    font-size:12px;   
    border-radius:6px;
    cursor:pointer;}
.btn-warning{background:#e74c3c;}
.btn-warning:hover{background:#c0392b;}
.status{margin-bottom:20px;font-weight:bold;padding:12px;border-radius:6px;background:#ecf0f1;border-left:5px solid #3498db;}
.status span{margin-left: 10px;}
</style>
</head>
<body>
<div class="container">
<h2>Device Configuration</h2>
<div class="status">Status: <span>)rawliteral"
                + statusMsg + R"rawliteral(</span></div>
<form id="configForm">

<h3>I. General & Location</h3>
<div class="form-row">
  <label for="deviceId">Device ID (Read-only):</label>
  <input type="text" id="deviceId" name="deviceId" readonly>
</div>
<div class="form-row"><label for="stationName">Station Name:</label><input type="text" id="stationName" name="stationName"></div>
<div class="form-row"><label for="stationDescription">Description:</label><input type="text" id="stationDescription" name="stationDescription"></div>
<div class="form-row"><label for="latitude">Latitude:</label><input type="number" step="0.000001" id="latitude" name="latitude"></div>
<div class="form-row"><label for="longitude">Longitude:</label><input type="number" step="0.000001" id="longitude" name="longitude"></div>

<h3>II. Network & WiFi</h3>
<div class="form-row"><label for="wifiSSID">WiFi SSID:</label><input type="text" id="wifiSSID" name="wifiSSID"></div>
<div class="form-row"><label for="wifiPass">WiFi Password:</label><input type="password" id="wifiPass" name="wifiPass"></div>


<h3>III. MQTT & Data Service</h3>
<div class="form-row">
  <label for="mqttEnabled">Enable MQTT:</label>
  <input type="checkbox" id="mqttEnabled" name="mqttEnabled">
</div>

<div class="form-row"><label for="mqttServer">MQTT Server:</label><input type="text" id="mqttServer" name="mqttServer"></div>
<div class="form-row"><label for="mqttPort">MQTT Port:</label><input type="number" id="mqttPort" name="mqttPort"></div>

<div class="form-row"><label for="mqttPass">Unified Token/API Key:</label><input type="text" id="mqttPass" name="mqttPass"></div>

<div class="form-row"><label for="queueMaxSize">Queue Max Size (bytes):</label><input type="number" id="queueMaxSize" name="queueMaxSize"></div>
<div class="form-row"><label for="queueFlushInterval">Queue Flush Interval (ms):</label><input type="number" id="queueFlushInterval" name="queueFlushInterval"></div>

<h3>IV. Timing & Power</h3>
<div class="form-row"><label for="sendInterval">Send Interval (ms):</label><input type="number" id="sendInterval" name="sendInterval"></div>
<div class="form-row"><label for="ntpServer">NTP Server:</label><input type="text" id="ntpServer" name="ntpServer"></div>
<div class="form-row"><label for="timeZone">Time Zone (UTC Offset):</label><input type="number" id="timeZone" name="timeZone"></div>
<div class="form-row"><label for="enableSleep">Enable Deep Sleep:</label><input type="checkbox" id="enableSleep" name="enableSleep"></div>
<div class="form-row"><label for="sleepDuration">Sleep Duration (seconds):</label><input type="number" id="sleepDuration" name="sleepDuration"></div>


<h3>V. Sensor Pinout & Enable/Disable</h3>
<div class="form-row">
  <label for="autoCalibrateOnBoot">Auto Calibrate on Boot:</label>
  <input type="checkbox" id="autoCalibrateOnBoot" name="autoCalibrateOnBoot">
</div>

<h4>Pinout</h4>
<div class="form-row"><label for="dustLEDPin">Dust LED Pin (GP2Y):</label><input type="number" id="dustLEDPin" name="dustLEDPin"></div>
<div class="form-row"><label for="dustADCPin">Dust ADC Pin (GP2Y):</label><input type="number" id="dustADCPin" name="dustADCPin"></div>
<div class="form-row"><label for="mqADCPin">MQ ADC Pin:</label><input type="number" id="mqADCPin" name="mqADCPin"></div>

<h4>PMS7003/PMSA003</h4>
<div class="form-row">
  <label for="pmsEnabled">Enable PMS Sensor:</label>
  <input type="checkbox" id="pmsEnabled" name="pmsEnabled">
</div>
<div class="form-row"><label for="pmsRxPin">PMS RX Pin:</label><input type="number" id="pmsRxPin" name="pmsRxPin"></div>
<div class="form-row"><label for="pmsTxPin">PMS TX Pin:</label><input type="number" id="pmsTxPin" name="pmsTxPin"></div>
<div class="form-row"><label for="pmsSetPin">PMS Set Pin (-1 if unused):</label><input type="number" id="pmsSetPin" name="pmsSetPin"></div>


<h3>VI. Sensor Calibration Values</h3>
<h4>MQ Gas Sensor</h4>
<div class="form-row"><label for="mq_rl_kohm">MQ RL (kOhm):</label><input type="number" step="0.01" id="mq_rl_kohm" name="mq_rl_kohm"></div>
<div class="form-row"><label for="mq_r0_ratio_clean">MQ R0/Rs ratio:</label><input type="number" step="0.001" id="mq_r0_ratio_clean" name="mq_r0_ratio_clean"></div>
<div class="form-row"><label for="mq_rzero">MQ RZERO (Persisted R0):</label><input type="number" step="0.01" id="mq_rzero" name="mq_rzero" readonly></div>

<h4>Dust Sensor</h4>
<div class="form-row"><label for="dust_baseline">Dust Baseline (Persisted V):</label><input type="number" step="0.0001" id="dust_baseline" name="dust_baseline" readonly></div>
<div class="form-row"><label for="dust_calibration">Dust calibration factor:</label><input type="number" step="0.0001" id="dust_calibration" name="dust_calibration"></div>

<div class="btn-group">
<button type="submit" class="btn-primary">Save & Reboot</button>

<button class="btn-warning" onclick="if(confirm('Are you sure you want to reboot the device?')) window.location.href='/reboot'">Reboot</button>
<button class="btn-warning" onclick="if(confirm('WARNING: This will erase ALL configuration and restart. Continue?')) window.location.href='/reset'">Factory Reset</button>
</div>

</form>

<script>
const configData = JSON.parse(')rawliteral"
                + jsonConfig + R"rawliteral(');

for(const key in configData) {
    const input = document.getElementById(key);
    if(input) {
        if(input.type === 'checkbox') {
            input.checked = configData[key] === true || configData[key] === "true";
        } else {

            // Special handling for read-only fields
            if (key === 'deviceId' || key === 'mq_rzero' || key === 'dust_baseline') {
                input.value = configData[key];
                continue;
            }
			input.value = configData[key];
            if (input.type === 'password' && input.value !== "") {
                input.placeholder = '****** (Set)';
                input.value = '';
            }
        }
    }
}

document.getElementById('configForm').onsubmit = function(e){
    e.preventDefault();
    const data = {};

    for (const input of this.elements) {
        if (!input.name) continue; 

        if (input.type === 'checkbox') {
            data[input.name] = input.checked; 
        } else if (input.type === 'number') {
            data[input.name] = parseFloat(input.value);
        } else if (input.type === 'password') {
            if(input.value !== '') data[input.name] = input.value; 
        } else {
            data[input.name] = input.value;
        }
    }

    fetch('/save',{
        method:'POST',
        headers:{'Content-Type':'application/json'},
        body:JSON.stringify(data)
    })
    .then(r=>r.text()).then(t=>alert(t));
};

</script>
</div>
</body>
</html>
)rawliteral";

  return html;
}
