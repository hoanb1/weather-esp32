// dashboard_js.h
#pragma once
#include <Arduino.h>

const char dashboard_js[] PROGMEM = R"rawliteral(

let tempData=[], humData=[], presData=[],
    pm25Data=[], aqiOverallData=[], mqData=[],
    pm10Data=[],
    pm25GP2YData=[],
    aqiPM10Data=[], aqiGP2YData=[], aqiVOCData=[]; // All data arrays remain

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

// --- Line Chart (UPDATED to accept multiple series) ---
const createLineChart = (containerId, seriesConfig) => {
    // seriesConfig is an array of { name: '...', unit: '...', color: '...', data: [] }
    const series = seriesConfig.map(config => ({
        name: config.name,
        data: config.data || [],
        color: config.color,
        marker: { enabled: false },
        lineWidth: 2,
        threshold: null,
        animation: false
    }));

    // Check if yAxis is needed (for AQI comparison, we use a custom yAxis to show zones)
    let yAxisOptions = {
        title: { text: null },
        gridLineColor: 'rgba(0, 0, 0, 0.05)',
        labels: { style: { color: '#95a5a6' } }
    };

    // For AQI Comparison chart, add AQI zones (same as gauge zones, but horizontal)
    if (containerId === 'chartAQIComparison') {
        const zonesAQIPlotBands = zonesAQI.map((z, i) => {
             const from = i === 0 ? 0 : zonesAQI[i - 1].max;
             return { from: from, to: z.max, color: z.color, opacity: 0.1, label: { text: zonesAQI[i-1] ? '' : 'AQI Zones', align: 'high', style: { color: '#95a5a6' } } };
        });
        zonesAQIPlotBands.unshift({ from: 0, to: 320, color: 'rgba(0, 0, 0, 0.05)' }); // Background

        yAxisOptions = Highcharts.merge(yAxisOptions, {
            plotBands: zonesAQIPlotBands,
            max: 320 // Ensure AQI charts are consistent
        });
    }

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
        yAxis: yAxisOptions,
        legend: { enabled: true, align: 'center', verticalAlign: 'top', layout: 'horizontal', itemStyle: { fontWeight: '400', color: 'var(--color-text-dark)' } },
        tooltip: {
            shared: true,
            xDateFormat: '%Y-%m-%d %H:%M:%S',
           useUTC: false

        },
        series: series
    });
};

const zonesTemp = [{max: 15, color: '#3498db'}, {max: 30, color: '#2ecc71'}, {max: 60, color: '#e74c3c'}]; 
const zonesHum = [{max: 30, color: '#f39c12'}, {max: 70, color: '#2ecc71'}, {max: 100, color: '#3498db'}]; 
const zonesPres = [{max: 720, color: '#f39c12'}, {max: 1050, color: '#2ecc71'}, {max: 1100, color: '#e74c3c'}]; 
const zonesPM25 = [{max: 12, color: '#2ecc71'}, {max: 35, color: '#f39c12'}, {max: 55, color: '#e67e22'}, {max: 150, color: '#e74c3c'}, {max: 320, color: '#9b59b6'}];
const zonesAQI = [{max: 50, color: '#2ecc71'}, {max: 100, color: '#f39c12'}, {max: 150, color: '#e67e22'}, {max: 200, color: '#e74c3c'}, {max: 320, color: '#9b59b6'}]; 
const zonesMQ = [{max: 200, color: '#2ecc71'}, {max: 500, color: '#f39c12'}, {max: 1000, color: '#e74c3c'}];

// --- Init Gauges ---
const chartTempGauge = createSolidGauge('gaugeTemp', 'Temperature', '', 0, 50, zonesTemp, 1);
const chartHumGauge = createSolidGauge('gaugeHum', 'Humidity', '', 0, 100, zonesHum, 0);
const chartPresGauge = createSolidGauge('gaugePres', 'Pressure', '', 800, 1100, zonesPres, 0);
const chartPM25Gauge = createSolidGauge('gaugePM25', 'PM2.5 (PMS)', '', 0, 250, zonesPM25, 0, true);
const chartAQIOverallGauge = createSolidGauge('gaugeAQIOverall', 'Overall AQI', '', 0, 300, zonesAQI, 0, true);
const chartMQGauge = createSolidGauge('gaugeMQ', 'MQ Gas Index', '', 0, 1000, zonesMQ, 0, true);

// --- Init Line Charts (Main 6 - Individual Charts remain) ---
const chartTemp = createLineChart('chartTemp', [{ name: 'Temperature (°C)', color: '#e74c3c' }]);
const chartHum = createLineChart('chartHum', [{ name: 'Humidity (%)', color: '#3498db' }]);
const chartPres = createLineChart('chartPres', [{ name: 'Pressure (hPa)', color: '#34495e' }]);
const chartPM25 = createLineChart('chartPM25', [{ name: 'PM2.5 (PMS) (µg/m³)', color: '#e67e22' }]);
const chartAQIOverall = createLineChart('chartAQIOverall', [{ name: 'Overall AQI', color: '#9b59b6' }]);
const chartMQ = createLineChart('chartMQ', [{ name: 'MQ Gas Index', color: '#f39c12' }]);

// --- Init NEW Combined Line Charts ---
const chartParticulateMatter = createLineChart('chartParticulateMatter', [
    { name: 'PM2.5 (PMS)', color: '#e67e22' },
    { name: 'PM10 (PMS)', color: '#f39c12' },
    { name: 'PM2.5 (GP2Y)', color: '#c0392b' }
]);

const chartAQIComparison = createLineChart('chartAQIComparison', [
    { name: 'AQI (PMS PM2.5)', color: '#9b59b6' },
    { name: 'AQI (PM10)', color: '#2980b9' },
    { name: 'AQI (GP2Y PM2.5)', color: '#7f8c8d' },
    { name: 'AQI (VOC)', color: '#27ae60' }
]);


// --- Helper function to update the Solid Gauge (No Change) ---
function updateGauge(gaugeChart, value, redraw = true) {
    if (gaugeChart && gaugeChart.series && gaugeChart.series[0] && gaugeChart.series[0].points[0]) {
        const point = gaugeChart.series[0].points[0];
        point.update(value, redraw);
    }
}

// --- Data Handling (No Change) ---
function addData(arr, ts, v){
    arr.push([ts, v]); 
    if(arr.length > maxPoints) arr.shift();
}

function getFilteredData(arr){
    const now = Date.now();
    const rangeMs = parseInt(document.getElementById('timeRangeSelect').value) * 1000;
    return arr.filter(d => d[0] >= now - rangeMs);
}

// --- UPDATED: Update Charts Function ---
function updateCharts(){
    const range = parseInt(document.getElementById('timeRangeSelect').value) * 1000;
    const now = Date.now();
    
    // Helper for single-series charts
    const setChartDataSingle = (chart, data, redraw) => {
        if(chart && chart.series && chart.series[0]) {
            chart.series[0].setData(data, redraw);
            chart.xAxis[0].setExtremes(now - range, now, redraw, false);
        }
    };

    // Helper for multi-series charts
    const setChartDataMulti = (chart, dataArrays, redraw) => {
        if(chart && chart.series) {
            dataArrays.forEach((data, index) => {
                if(chart.series[index]) {
                    chart.series[index].setData(data, false); // Set data without redraw
                }
            });
            chart.xAxis[0].setExtremes(now - range, now, false, false);
            chart.redraw(redraw); // Redraw once
        }
    };


    // Individual Charts (Main 6)
    setChartDataSingle(chartTemp, getFilteredData(tempData), false);
    setChartDataSingle(chartHum, getFilteredData(humData), false);
    setChartDataSingle(chartPres, getFilteredData(presData), false);
    setChartDataSingle(chartPM25, getFilteredData(pm25Data), false);
    setChartDataSingle(chartAQIOverall, getFilteredData(aqiOverallData), false);
    setChartDataSingle(chartMQ, getFilteredData(mqData), false);

    // Combined Particulate Matter Chart
    setChartDataMulti(chartParticulateMatter, [
        getFilteredData(pm25Data),
        getFilteredData(pm10Data),
        getFilteredData(pm25GP2YData)
    ], false); // Redraw will be triggered after all multi-sets

    // Combined AQI Comparison Chart
    setChartDataMulti(chartAQIComparison, [
        getFilteredData(aqiOverallData),
        getFilteredData(aqiPM10Data),
        getFilteredData(aqiGP2YData),
        getFilteredData(aqiVOCData)
    ], true); // Final redraw for all multi-series charts

    // Redraw all individual charts (not strictly needed if no change but good practice)
    chartTemp.redraw(true);
    chartHum.redraw(true);
    chartPres.redraw(true);
    chartPM25.redraw(true);
    chartAQIOverall.redraw(true);
    chartMQ.redraw(true);
}

// --- LocalStorage Logic (No Change) ---
function saveTimeRange(value) {
    try { localStorage.setItem(TIME_RANGE_STORAGE_KEY, value); } catch (e) {}
}
function loadTimeRange() {
    try {
        const val = localStorage.getItem(TIME_RANGE_STORAGE_KEY);
        if (val) document.getElementById('timeRangeSelect').value = val;
    } catch (e) {}
}

// --- WebSocket (Data processing section updated for new arrays) ---

function connectWS(){
    ws = new WebSocket("/ws");
    
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
    if (d.type !== "log" && (d.t !== undefined || d.h !== undefined || d.pm25_pms !== undefined)) {

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

        // --- UPDATED to use new structure fields (Main 6) ---
        if (d.pm25_pms !== undefined) {
            addData(pm25Data, ts, d.pm25_pms);
            updateGauge(chartPM25Gauge, d.pm25_pms);
            isNewData = true;
        }
        if (d.aqi !== undefined) {
            addData(aqiOverallData, ts, d.aqi);
            updateGauge(chartAQIOverallGauge, d.aqi);
            isNewData = true;
        }
        if (d.mq !== undefined) {
            addData(mqData, ts, d.mq);
            updateGauge(chartMQGauge, d.mq);
            isNewData = true;
        }

        // --- NEW data fields for line charts only (Additional) ---
        if (d.pm10_pms !== undefined) {
            addData(pm10Data, ts, d.pm10_pms);
            isNewData = true;
        }
        if (d.aqi_pms10 !== undefined) {
            addData(aqiPM10Data, ts, d.aqi_pms10);
            isNewData = true;
        }
        if (d.pm25_gp2y !== undefined) {
            addData(pm25GP2YData, ts, d.pm25_gp2y);
            isNewData = true;
        }
        if (d.aqi_gp2y25 !== undefined) {
            addData(aqiGP2YData, ts, d.aqi_gp2y25);
            isNewData = true;
        }
        if (d.aqi_voc !== undefined) {
            addData(aqiVOCData, ts, d.aqi_voc);
            isNewData = true;
        }
    }

    // --- Logs ---
    if (d.msg !== undefined || d.type === "log") {
        let log = document.getElementById("logArea");
        const now = new Date();
        const timeStr =
            now.toLocaleTimeString("en-GB")
            //+"."
            //+String(now.getMilliseconds()).padStart(3, "0")
            ;
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