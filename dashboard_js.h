#pragma once
#include <Arduino.h>

const char dashboard_js[] PROGMEM = R"rawliteral(
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
    if(d.mqr!==undefined){
        document.getElementById('mqVal').innerText=d.mqr.toFixed(0); 
        addData(mqData,ts_us,d.mqr);
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
)rawliteral";
