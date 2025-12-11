#pragma once
#include <Arduino.h>

const char dashboard_js[] PROGMEM = R"rawliteral(


let tempData=[], humData=[], presData=[], dustData=[], aqiData=[], mqData=[];
const maxPoints = 120;
const TIME_RANGE_STORAGE_KEY = 'chartTimeRange';

let ws;

// --- Highcharts Configuration ---
Highcharts.setOptions({
   time: {
        useUTC: false
    },
    global: {
        useUTC: false
    },
    chart: {
        style: { fontFamily: 'Poppins, sans-serif' },
        backgroundColor: 'white' 
    },
    title: { text: null },
    credits: { enabled: false },
    exporting: { enabled: false }
});

// --- Base Gauge ---
const baseGaugeOptions = {
    chart: {
        type: 'solidgauge',
        height: 150,
        backgroundColor: 'white'
    },
    title: null,
    pane: {
        center: ['50%', '85%'],
        size: '140%',
        startAngle: -90,
        endAngle: 90,
        background: {
            backgroundColor: 'var(--color-background, #f0f2f5)',
            borderColor: 'transparent',
            borderRadius: 5,
            innerRadius: '60%',
            outerRadius: '86%',
            shape: 'arc'
        }
    },
    exporting: { enabled: false },
    tooltip: { enabled: false },
    yAxis: {
        lineWidth: 0,
        tickWidth: 0,
        minorTickInterval: null,
        tickAmount: 2,
        labels: { y: 16, distance: 10, style: { color: 'var(--color-text-muted)' } },
        title: { y: -70 }
    },
    plotOptions: {
        solidgauge: {
            borderRadius: 3,
            dataLabels: {
                y: 5,
                borderWidth: 0,
                useHTML: true
            }
        }
    }
};

// --- Gauges ---
function createSolidGauge(containerId, title, unit, min, max, zones, formatDecimals, isReversed = false) {

    const highchartsZones = zones.map(z => ({ value: z.max, color: z.color }));
    const lastZone = highchartsZones[highchartsZones.length - 1];
    if (lastZone && lastZone.value < max) {
        highchartsZones.push({ value: max, color: lastZone.color });
    }

    return Highcharts.chart(containerId, Highcharts.merge(baseGaugeOptions, {
        yAxis: {
            min: min,
            max: max,
            title: { text: null },
            stops: highchartsZones.map(zone => {
                const p = (zone.value - min) / (max - min);
                return [p, zone.color];
            }),
            plotBands: zones.map((z, i) => {
                const from = i === 0 ? min : zones[i - 1].max;
                return { from: from, to: z.max, color: z.color };
            })
        },
        series: [{
            name: title,
            radius: '86%',
            data: [0],
            dataLabels: {
                format:
                '<div style="text-align:center; margin-top:-10px;">' +
                '<span style="font-size:2.2em; font-weight:700;">{y:.' + formatDecimals + 'f}</span>' +
                '</div>'
            },
            tooltip: { valueSuffix: '' }, // removed unit
            innerRadius: '60%',
            outerRadius: '100%',
            linecap: 'round',
            color: highchartsZones[0].color
        }]
    }));
}

// --- Line Chart (units removed) ---
const createLineChart = (containerId, title, unit, color) => {
    return Highcharts.chart(containerId, {
        chart: {
            type: 'spline',
            zoomType: 'x',
           margin: [20, 20, 40, 50],
            events: {
                load: function () {
                    setTimeout(() => this.reflow(), 100);
                }
            }
        },
        xAxis: {
            type: 'datetime',
            lineColor: '#ecf0f1',
            tickColor: '#ecf0f1',
            dateTimeLabelFormats: {
                millisecond: '%H:%M:%S.%L',
                second: '%H:%M:%S',
                minute: '%H:%M',
                hour: '%H:%M',
                day: '%e. %b',
                week: '%e. %b',
                month: '%b \'%y',
                year: '%Y'
            },
            labels: {
                rotation: -15,
                align: 'right',              
                style: { color: '#95a5a6' }
            }
        },
        yAxis: {
            title: { text: null },
            gridLineColor: 'rgba(0, 0, 0, 0.05)',
            labels: { style: { color: '#95a5a6' } }
        },
        legend: { enabled: false },
        tooltip: {
            shared: true,
            xDateFormat: '%Y-%m-%d %H:%M:%S',
           useUTC: false

        },
        series: [{
            name: title,
            data: [],
            color: color,
            marker: { enabled: false },
            lineWidth: 2,
            threshold: null,
            animation: false
        }]
    });
};

const zonesTemp = [{max: 15, color: '#3498db'}, {max: 30, color: '#2ecc71'}, {max: 60, color: '#e74c3c'}]; 
const zonesHum = [{max: 30, color: '#f39c12'}, {max: 70, color: '#2ecc71'}, {max: 100, color: '#3498db'}]; 
const zonesPres = [{max: 720, color: '#f39c12'}, {max: 1050, color: '#2ecc71'}, {max: 1100, color: '#e74c3c'}]; 
const zonesDust = [{max: 12, color: '#2ecc71'}, {max: 35, color: '#f39c12'}, {max: 55, color: '#e67e22'}, {max: 150, color: '#e74c3c'}, {max: 320, color: '#9b59b6'}]; 
const zonesAQI = [{max: 50, color: '#2ecc71'}, {max: 100, color: '#f39c12'}, {max: 150, color: '#e67e22'}, {max: 200, color: '#e74c3c'}, {max: 320, color: '#9b59b6'}]; 
const zonesMQ = [{max: 200, color: '#2ecc71'}, {max: 500, color: '#f39c12'}, {max: 1000, color: '#e74c3c'}];

// --- Init Gauges ---
const chartTempGauge = createSolidGauge('gaugeTemp', 'Temperature', '', 0, 50, zonesTemp, 1);
const chartHumGauge = createSolidGauge('gaugeHum', 'Humidity', '', 0, 100, zonesHum, 0);
const chartPresGauge = createSolidGauge('gaugePres', 'Pressure', '', 800, 1100, zonesPres, 0);
const chartDustGauge = createSolidGauge('gaugeDust', 'PM2.5', '', 0, 250, zonesDust, 0, true);
const chartAQIGauge = createSolidGauge('gaugeAQI', 'AQI', '', 0, 300, zonesAQI, 0, true);
const chartMQGauge = createSolidGauge('gaugeMQ', 'MQ Index', '', 0, 1000, zonesMQ, 0, true);

// --- Init Line Charts ---
const chartTemp = createLineChart('chartTemp', 'Temperature', '', '#e74c3c');
const chartHum = createLineChart('chartHum', 'Humidity', '', '#3498db');
const chartPres = createLineChart('chartPres', 'Pressure', '', '#34495e');
const chartDust = createLineChart('chartDust', 'PM2.5', '', '#e67e22');
const chartAQI = createLineChart('chartAQI', 'AQI', '', '#9b59b6');
const chartMQ = createLineChart('chartMQ', 'MQ Index', '', '#f39c12');


// --- Helper function to update the Solid Gauge ---
function updateGauge(gaugeChart, value, redraw = true) {
    if (gaugeChart && gaugeChart.series && gaugeChart.series[0] && gaugeChart.series[0].points[0]) {
        const point = gaugeChart.series[0].points[0];
        point.update(value, redraw);
    }
}

// --- Data Handling (No change) ---
function addData(arr, ts, v){
    arr.push([ts, v]); 
    if(arr.length > maxPoints) arr.shift();
}

function getFilteredData(arr){
    const now = Date.now();
    const rangeMs = parseInt(document.getElementById('timeRangeSelect').value) * 1000;
    return arr.filter(d => d[0] >= now - rangeMs);
}

function updateCharts(){
    const range = parseInt(document.getElementById('timeRangeSelect').value) * 1000;
    const now = Date.now();
    
    const setChartData = (chart, data, redraw) => {
        if(chart && chart.series && chart.series[0]) {
            chart.series[0].setData(data, redraw);
            chart.xAxis[0].setExtremes(now - range, now, redraw, false);
        }
    };

    // IMPORTANT: All must be 'true' to redraw individual charts
    setChartData(chartTemp, getFilteredData(tempData), true);
    setChartData(chartHum, getFilteredData(humData), true);
    setChartData(chartPres, getFilteredData(presData), true);
    setChartData(chartDust, getFilteredData(dustData), true);
    setChartData(chartAQI, getFilteredData(aqiData), true);
    setChartData(chartMQ, getFilteredData(mqData), true); 
}

// --- LocalStorage Logic (No change) ---
function saveTimeRange(value) {
    try { localStorage.setItem(TIME_RANGE_STORAGE_KEY, value); } catch (e) {}
}
function loadTimeRange() {
    try {
        const val = localStorage.getItem(TIME_RANGE_STORAGE_KEY);
        if (val) document.getElementById('timeRangeSelect').value = val;
    } catch (e) {}
}

// --- WebSocket (UPDATED to use Highcharts Gauge Update) ---

function connectWS(){
    ws = new WebSocket("ws://"+location.hostname+"/ws");
    
    ws.onopen = ()=>{
       // console.log("WebSocket connected");
      
        loadTimeRange();
        window.dispatchEvent(new Event('resize')); 
        const s = document.getElementById('statusIndicator');
        s.innerText='Connected'; s.style.backgroundColor='#2ecc71'; 
    };
    
    ws.onmessage = e => {

    let d;
    try { d = JSON.parse(e.data); } catch(err) { return; }

   let ts = Date.now();   // fallback

    if (d.ts !== undefined && typeof d.ts === "number" && d.ts > 0) {

        const ts_ms = Math.floor(d.ts / 1000);  // UTC ms
      

        // convert UTC -> local
        const local_ts = ts_ms + (new Date().getTimezoneOffset() * -60000);
      

        const now = Date.now();

        const maxFutureDrift = 10 * 60 * 1000;
        const maxPastDrift   = 24 * 60 * 60 * 1000;

        if (
            local_ts > now - maxPastDrift &&
            local_ts < now + maxFutureDrift
        ) {
            ts = local_ts;
        } 
    } 



    let isNewData = false;

    // --- Sensor data ---
    if (d.type !== "log" && (d.t !== undefined || d.h !== undefined)) {

        if (d.t !== undefined) {
            addData(tempData, ts, d.t);
            updateGauge(chartTempGauge, d.t);
            isNewData = true;
        }
        if (d.h !== undefined) {
            addData(humData, ts, d.h);
            updateGauge(chartHumGauge, d.h);
            isNewData = true;
        }
        if (d.p !== undefined) {
            addData(presData, ts, d.p);
            updateGauge(chartPresGauge, d.p);
            isNewData = true;
        }
        if (d.pm !== undefined) {
            addData(dustData, ts, d.pm);
            updateGauge(chartDustGauge, d.pm);
            isNewData = true;
        }
        if (d.aqi !== undefined) {
            addData(aqiData, ts, d.aqi);
            updateGauge(chartAQIGauge, d.aqi);
            isNewData = true;
        }
        if (d.mq !== undefined) {
            addData(mqData, ts, d.mq);
            updateGauge(chartMQGauge, d.mq);
            isNewData = true;
        }
    }

    // --- Logs ---
    if (d.msg !== undefined || d.type === "log") {
        let log = document.getElementById("logArea");
        const now = new Date();
        const timeStr =
            now.toLocaleTimeString("en-GB") +
            "." +
            String(now.getMilliseconds()).padStart(3, "0");
        const msg = d.msg ? d.msg : JSON.stringify(d);
        log.innerText += `[${timeStr}] ${msg}\n`;
        log.scrollTop = log.scrollHeight;
    }

    // --- System info ---
    if (d.uptime_seconds !== undefined || d.status_msg !== undefined || d.wifi_rssi !== undefined) {

        if (d.uptime_seconds !== undefined) {
            const u = d.uptime_seconds;
            const h = Math.floor(u / 3600);
            const m = Math.floor((u % 3600) / 60);
            const s = Math.floor(u % 60);
            let rssi = d.wifi_rssi !== undefined ? ` | RSSI: ${d.wifi_rssi} dBm` : "";
            document.getElementById("sysInfoVal").innerText =
                `Uptime: ${h}h ${m}m ${s}s${rssi}`;
        }

        if (d.status_msg !== undefined) {
            const s = document.getElementById("statusIndicator");
            s.innerText = d.status_msg;
            if (d.status_msg.includes("Connected")) s.style.backgroundColor = "#2ecc71";
            else if (d.status_msg.includes("Connecting")) s.style.backgroundColor = "#f39c12";
            else s.style.backgroundColor = "#e74c3c";
        }

        if (d.wifi_rssi !== undefined && d.uptime_seconds === undefined) {
            const txt = document.getElementById("sysInfoVal").innerText;
            const newRssi = ` | RSSI: ${d.wifi_rssi} dBm`;
            if (txt.includes("Uptime:")) {
                document.getElementById("sysInfoVal").innerText =
                    txt.replace(/ \| RSSI: .*|$/, newRssi);
            }
        }
    }

    if (isNewData) updateCharts();
};
    
    ws.onclose = e => {
       
        setTimeout(connectWS, 2000);
    };
    ws.onerror = e => ws.close();
}

// --- Init (No change) ---
document.addEventListener('DOMContentLoaded', () => {
    const btn = document.getElementById('logToggleBtn');
    const wrapper = document.getElementById('logAreaWrapper');
    if(btn && wrapper) {
        btn.addEventListener('click', () => {
            const exp = wrapper.classList.toggle('expanded');
            btn.innerText = exp ? 'Hide Logs' : 'Show Logs';
        });
    }

    loadTimeRange(); 
    connectWS();

    const sel = document.getElementById('timeRangeSelect');
    if(sel) sel.addEventListener('change', (e) => {
        saveTimeRange(e.target.value); 
        updateCharts();
    });
});
)rawliteral";