//dashboard_js.h

#pragma once
#include <Arduino.h>

const char dashboard_js[] PROGMEM = R"rawliteral(

let tempData=[], humData=[], presData=[], dustData=[], aqiData=[], mqData=[];
const maxPoints = 120;

function addData(arr, ts, v){
    arr.push({x: ts/1000, y: v});
    if(arr.length > maxPoints) arr.shift();
}

function createChart(ctx, data, color, label){
    return new Chart(ctx,{
        type:'line',
        data:{
            datasets:[{
                label:label,
                data:data,
                borderColor:color,
                borderWidth:1,
                fill:false,
                tension:0.4,
                pointRadius:0
            }]
        },
        options:{
            animation:{duration:0},
            scales:{
                x:{type:'time',time:{unit:'second'}},
                y:{beginAtZero:false}
            }
        }
    });
}

function createTempHumChart(ctx, tempData, humData){
    return new Chart(ctx,{
        type:'line',
        data:{
            datasets:[
            {
                label:'Temperature (°C)',
                data:tempData,
                borderColor:'#e74c3c',
                borderWidth:1,
                yAxisID:'yTemp',
                tension:0.4,
                pointRadius:0
            },
            {
                label:'Humidity (%)',
                data:humData,
                borderColor:'#3498db',
                borderWidth:1,
                yAxisID:'yHum',
                tension:0.4,
                pointRadius:0
            }]
        },
        options:{
            animation:{duration:0},
            scales:{
                x:{type:'time',time:{unit:'second'}},
                yTemp:{type:'linear',position:'left'},
                yHum:{type:'linear',position:'right',min:0,max:100,grid:{drawOnChartArea:false}}
            }
        }
    });
}

// Create charts
const chartTempHum = createTempHumChart(
    document.getElementById("chartTempHum").getContext("2d"), tempData, humData);
const chartPres = createChart(document.getElementById("chartPres").getContext("2d"), presData, '#2980b9', 'Pressure');
const chartDust = createChart(document.getElementById("chartDust").getContext("2d"), dustData, '#8e5a2a', 'PM2.5');
const chartAQI = createChart(document.getElementById("chartAQI").getContext("2d"), aqiData, '#8e44ad', 'AQI');
const chartMQ = createChart(document.getElementById("chartMQ").getContext("2d"), mqData, '#f39c12', 'MQ Index');

// WebSocket data flow

let ws;

function connectWS() {
    ws = new WebSocket("ws://" + location.hostname + "/ws");

    ws.onopen = () => {
        console.log("WebSocket connected");
    };

    ws.onmessage = e=>{
        const d = JSON.parse(e.data);
        const ts = d.ts;

        if(d.t !== undefined && d.h !== undefined){
            document.getElementById("tempHumVal").innerText =
                d.t.toFixed(1) + "°C / " + d.h.toFixed(1) + "%";
            addData(tempData, ts, d.t);
            addData(humData, ts, d.h);
            chartTempHum.update();
        }

        if(d.p !== undefined){
            document.getElementById("presVal").innerText = d.p.toFixed(1) + " hPa";
            addData(presData, ts, d.p);
            chartPres.update();
        }

        if(d.pm !== undefined){
            document.getElementById("dustVal").innerText =
                d.pm.toFixed(1) + " µg/m³";
            addData(dustData, ts, d.pm);
            chartDust.update();
        }

        if(d.aqi !== undefined){
            document.getElementById("aqiVal").innerText = d.aqi;
            addData(aqiData, ts, d.aqi);
            chartAQI.update();
        }

        if(d.mq !== undefined){
            document.getElementById("mqVal").innerText = d.mq;
            addData(mqData, ts, d.mq);
            chartMQ.update();
        }

        if(d.msg !== undefined){
            let log=document.getElementById("logArea");
            log.innerText += d.msg + "\n";
            log.scrollTop = log.scrollHeight;
        }

        if(d.uptime_seconds !== undefined || d.status_msg !== undefined){

            if(d.uptime_seconds !== undefined){
                const uptime = d.uptime_seconds;
                const h = Math.floor(uptime / 3600);
                const m = Math.floor((uptime % 3600) / 60);
                const s = Math.floor(uptime % 60);
                
                let rssi = d.wifi_rssi !== undefined ? ` | RSSI: ${d.wifi_rssi} dBm` : '';

                document.getElementById('sysInfoVal').innerText = `Uptime: ${h}h ${m}m ${s}s${rssi}`;
            }

            if(d.status_msg !== undefined){
                const statusEl = document.getElementById('statusIndicator');
                statusEl.innerText = d.status_msg;
                
                if (d.status_msg.includes("Connected")) {
                    statusEl.style.color = 'green';
                } else if (d.status_msg.includes("Connecting")) {
                    statusEl.style.color = '#f39c12'; 
                } else {
                    statusEl.style.color = 'red';
                }
            }
        }
    }

    ws.onclose = e => {
        console.log("WebSocket closed, reconnecting in 2s...");
        
        let log=document.getElementById("logArea");
        log.innerText +=  "WebSocket closed, reconnecting in 2s...\n";
        log.scrollTop = log.scrollHeight;
        setTimeout(connectWS, 2000);
    };

    ws.onerror = e => {
        console.log("WebSocket error", e);
        ws.close();
    };
}

connectWS();

)rawliteral";
