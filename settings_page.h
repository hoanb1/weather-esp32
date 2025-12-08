#pragma once
#include <Arduino.h>
String getSettingsPageHTML(const String &statusMsg, const String &jsonConfig) {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
body{font-family:Arial;background:#f4f7f6;padding:20px;color:#333;}
.container{max-width:500px;margin:0 auto;background:#fff;padding:20px;border-radius:10px;box-shadow:0 4px 10px rgba(0,0,0,0.1);}
h2{color:#2c3e50;border-bottom:2px solid #3498db;padding-bottom:10px;}
.form-row{display:flex;align-items:center;margin-top:10px;}
.form-row label{width:40%;font-weight:bold;}
.form-row input[type=text],.form-row input[type=password],.form-row input[type=number]{width:60%;padding:10px;border:1px solid #ccc;border-radius:5px;}
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
<div class="form-row"><label for="deviceId">Device ID:</label><input type="text" id="deviceId" name="deviceId"></div>
<div class="form-row"><label for="latitude">Latitude:</label><input type="number" step="0.000001" id="latitude" name="latitude"></div>
<div class="form-row"><label for="longitude">Longitude:</label><input type="number" step="0.000001" id="longitude" name="longitude"></div>
<div class="form-row"><label for="wifiSSID">WiFi SSID:</label><input type="text" id="wifiSSID" name="wifiSSID"></div>
<div class="form-row"><label for="wifiPass">WiFi Password:</label><input type="text" id="wifiPass" name="wifiPass"></div>
<div class="form-row"><label for="mqttServer">MQTT Server:</label><input type="text" id="mqttServer" name="mqttServer"></div>
<div class="form-row"><label for="mqttPort">MQTT Port:</label><input type="number" id="mqttPort" name="mqttPort"></div>
<div class="form-row"><label for="mqttUser">MQTT User:</label><input type="text" id="mqttUser" name="mqttUser"></div>
<div class="form-row"><label for="mqttPass">MQTT Pass:</label><input type="text" id="mqttPass" name="mqttPass"></div>
<div class="form-row"><label for="mqttTopic">MQTT Topic:</label><input type="text" id="mqttTopic" name="mqttTopic"></div>
<div class="form-row"><label for="sendInterval">Send Interval (ms):</label><input type="number" id="sendInterval" name="sendInterval"></div>
<div class="form-row"><label for="ntpServer">NTP Server:</label><input type="text" id="ntpServer" name="ntpServer"></div>
<div class="form-row"><label for="dustLEDPin">Dust LED Pin:</label><input type="number" id="dustLEDPin" name="dustLEDPin"></div>
<div class="form-row"><label for="dustADCPin">Dust ADC Pin:</label><input type="number" id="dustADCPin" name="dustADCPin"></div>
<div class="form-row"><label for="mqADCPin">MQ ADC Pin:</label><input type="number" id="mqADCPin" name="mqADCPin"></div>
<div class="form-row"><label for="mq_rl_kohm">MQ RL (kOhm):</label><input type="number" step="0.01" id="mq_rl_kohm" name="mq_rl_kohm"></div>
<div class="form-row"><label for="mq_r0_ratio_clean">MQ R0 ratio:</label><input type="number" step="0.001" id="mq_r0_ratio_clean" name="mq_r0_ratio_clean"></div>
<div class="form-row"><label for="tvoc_a_curve">TVOC Curve A:</label><input type="number" step="0.001" id="tvoc_a_curve" name="tvoc_a_curve"></div>
<div class="form-row"><label for="tvoc_b_curve">TVOC Curve B:</label><input type="number" step="0.001" id="tvoc_b_curve" name="tvoc_b_curve"></div>
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


String getOTAPageHTML(const String &statusMsg) {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
body{font-family:Arial;background:#f4f7f6;padding:20px;color:#333;}
.container{max-width:500px;margin:0 auto;background:#fff;padding:20px;border-radius:10px;box-shadow:0 4px 10px rgba(0,0,0,0.1);}
h2{color:#2c3e50;border-bottom:2px solid #3498db;padding-bottom:10px;}
button{background:#2ecc71;color:white;padding:10px 15px;border:none;border-radius:5px;cursor:pointer;margin-top:10px;font-size:16px;}
button:hover{background:#27ae60;}
input[type=file], input[type=text]{width:100%;padding:10px;margin-top:10px;border:1px solid #ccc;border-radius:5px;}
.status{margin-bottom:20px;font-weight:bold;padding:10px;border-radius:5px;background:#ecf0f1;}
</style>
</head>
<body>
<div class="container">
<h2>OTA Firmware Update</h2>
<div class="status">)rawliteral"
                + statusMsg + R"rawliteral(</div>

<!-- Upload file -->
<form method="POST" action="/update-file" enctype="multipart/form-data">
  <input type="file" name="update">
  <button type="submit">Upload & Update</button>
</form>

<!-- Update from URL -->
<form id="otaUrlForm">
  <label for="otaUrl">Firmware URL:</label>
  <input type="text" id="otaUrl" name="otaUrl" placeholder="https://hoan.uk/firmware.bin">
  <button type="button" onclick="updateFromUrl()">Update from URL</button>
</form>

<div id="logArea"></div>

<script>
function updateFromUrl() {
  const url = document.getElementById('otaUrl').value;
  if(!url) { alert('Enter a URL'); return; }
  fetch('/update-url', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({url: url})
  }).then(r=>r.text()).then(t=>{
    document.getElementById('logArea').innerText = t;
  });
}
</script>
</div>
</body>
</html>
)rawliteral";
  return html;
}
