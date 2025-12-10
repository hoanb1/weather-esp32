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
body{font-family:'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;background:#e9eef2;padding:20px;color:#333;}
.container{max-width:600px;margin:0 auto;background:#fff;padding:25px;border-radius:12px;box-shadow:0 6px 16px rgba(0,0,0,0.1);}
h2{color:#2c3e50;border-bottom:2px solid #3498db;padding-bottom:10px;margin-top:0;}
h3{color:#34495e;margin-top:20px;padding-bottom:5px;border-bottom:1px dashed #ccc;}
.form-row{display:flex;justify-content:space-between;align-items:center;margin-bottom:12px;}
.form-row label{width:45%;font-weight:600;color:#555;}
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

<h3>General & Network</h3>
<div class="form-row"><label for="deviceId">Device ID:</label><input type="text" id="deviceId" name="deviceId"></div>
<div class="form-row"><label for="latitude">Latitude:</label><input type="number" step="0.000001" id="latitude" name="latitude"></div>
<div class="form-row"><label for="longitude">Longitude:</label><input type="number" step="0.000001" id="longitude" name="longitude"></div>
<div class="form-row"><label for="wifiSSID">WiFi SSID:</label><input type="text" id="wifiSSID" name="wifiSSID"></div>
<div class="form-row"><label for="wifiPass">WiFi Password:</label><input type="password" id="wifiPass" name="wifiPass"></div>
<div class="form-row"><label for="ntpServer">NTP Server:</label><input type="text" id="ntpServer" name="ntpServer"></div>

<h3>MQTT Settings</h3>
<div class="form-row">
  <label for="mqttEnabled">Enable MQTT:</label>
  <input type="checkbox" id="mqttEnabled" name="mqttEnabled">
</div>

<div class="form-row"><label for="mqttServer">MQTT Server:</label><input type="text" id="mqttServer" name="mqttServer"></div>
<div class="form-row"><label for="mqttPort">MQTT Port:</label><input type="number" id="mqttPort" name="mqttPort"></div>
<div class="form-row"><label for="mqttUser">MQTT User:</label><input type="text" id="mqttUser" name="mqttUser"></div>
<div class="form-row"><label for="mqttPass">MQTT Password:</label><input type="password" id="mqttPass" name="mqttPass"></div>
<div class="form-row"><label for="mqttTopic">MQTT Topic:</label><input type="text" id="mqttTopic" name="mqttTopic"></div>
<div class="form-row"><label for="sendInterval">Send Interval (ms):</label><input type="number" id="sendInterval" name="sendInterval"></div>

<h3>Sensor Pinout & Calibration</h3>
<div class="form-row"><label for="dustLEDPin">Dust LED Pin:</label><input type="number" id="dustLEDPin" name="dustLEDPin"></div>
<div class="form-row"><label for="dustADCPin">Dust ADC Pin:</label><input type="number" id="dustADCPin" name="dustADCPin"></div>
<div class="form-row"><label for="mqADCPin">MQ ADC Pin:</label><input type="number" id="mqADCPin" name="mqADCPin"></div>
<div class="form-row"><label for="mq_rl_kohm">MQ RL (kOhm):</label><input type="number" step="0.01" id="mq_rl_kohm" name="mq_rl_kohm"></div>
<div class="form-row"><label for="mq_r0_ratio_clean">MQ R0 ratio:</label><input type="number" step="0.001" id="mq_r0_ratio_clean" name="mq_r0_ratio_clean"></div>
<div class="form-row"><label for="mq_rzero">MQ RZERO:</label><input type="number" step="0.01" id="mq_rzero" name="mq_rzero"></div>

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

String getOTAPageHTML(const String &statusMsg) {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>OTA Update</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
body{font-family:'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;background:#e9eef2;padding:20px;color:#333;}
.container{max-width:500px;margin:0 auto;background:#fff;padding:25px;border-radius:12px;box-shadow:0 6px 16px rgba(0,0,0,0.1);}
h2{color:#2c3e50;border-bottom:2px solid #3498db;padding-bottom:10px;margin-top:0;}
button{background:#2ecc71;color:white;padding:12px 15px;border:none;border-radius:6px;cursor:pointer;margin-top:10px;font-size:16px;width:100%;transition:background 0.3s;}
button:hover{background:#27ae60;}
label{display:block;margin-top:15px;font-weight:600;color:#555;}
input[type=file], input[type=text]{width:100%;padding:10px;margin-top:5px;border:1px solid #ddd;border-radius:6px;box-sizing:border-box;}
.status{margin-bottom:20px;font-weight:bold;padding:12px;border-radius:6px;background:#ecf0f1;border-left:5px solid #3498db;}
#logArea{height:150px;overflow:auto;background:#1c1c1c;color:#00ff66;padding:10px;font-family:monospace;margin-top:20px;border-radius:6px;}
</style>
</head>
<body>
<div class="container">
<h2>Firmware Update (OTA)</h2>
<div class="status">Status: )rawliteral"
                + statusMsg + R"rawliteral(</div>

<form method="POST" action="/update-file" enctype="multipart/form-data">
  <label for="updateFile">1. Local File Upload</label>
  <input type="file" name="update" id="updateFile" accept=".bin">
  <button type="submit">Upload & Update</button>
</form>

<form id="otaUrlForm">
  <label for="otaUrl">2. Update from URL (HTTP/HTTPS)</label>
  <input type="text" id="otaUrl" name="otaUrl" placeholder="e.g., https://your-server.com/firmware.bin">
  <button type="button" onclick="updateFromUrl()">Start Update from URL</button>
</form>

<div id="logArea">OTA log will appear here...</div>

<script>
function updateFromUrl() {
  const url = document.getElementById('otaUrl').value;
  const logArea = document.getElementById('logArea');
  if(!url) { alert('Please enter a valid URL'); return; }
  
  logArea.innerText = 'Starting OTA update from: ' + url + '...';

  fetch('/update-url', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({url: url})
  }).then(r=>r.text()).then(t=>{
    logArea.innerText += '\n' + t;
    if (t.includes("Starting")) {
        logArea.innerText += "\nCheck serial monitor for detailed progress.";
    }
  }).catch(e => {
    logArea.innerText = 'Error during fetch: ' + e;
  });
}
</script>
</div>
</body>
</html>
)rawliteral";
  return html;
}
