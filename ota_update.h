#pragma once
#include <Arduino.h>

void setupOTA();

inline String getOTAPageHTML(const String &statusMsg) {
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
#logArea{height:150px;overflow:auto;background:#1c1c1c;color:#00ff66;padding:10px;font-family:monospace;margin-top:20px;border-radius:6px;white-space: pre-wrap;}
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
  <input type="text" id="otaUrl" name="otaUrl" placeholder="e.g., https://hoan.uk/esp32/weather.lastest.bin">
  <button type="button" onclick="updateFromUrl()">Start Update from URL</button>
</form>

<pre id="logArea">OTA log will appear here...</div>

<script>
let ws;

function connectWS() {
  ws = new WebSocket("ws://" + location.host + "/ws");

  ws.onmessage = (event) => {
    try {
      const obj = JSON.parse(event.data);
      if (obj.type === "log") {
        const logArea = document.getElementById("logArea");
        logArea.innerText += obj.msg+"\n";
        logArea.scrollTop = logArea.scrollHeight;
      }
    } catch(e) {}
  };

  ws.onclose = () => {
    setTimeout(connectWS, 1000);
  };
}

connectWS();

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
  }).catch(e => {
    logArea.innerText = 'Fetch error: ' + e;
  });
}
</script>

</div>
</body>
</html>
)rawliteral";
  return html;
}
