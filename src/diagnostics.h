#pragma once
#include <Arduino.h>

const char DIAGNOSTICS[] PROGMEM = R"html(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>HX711 diagnostics</title>
<style>
body{font:16px system-ui;background:#111;color:#eee;max-width:800px;margin:24px auto;padding:16px}
a{color:#4ade80}table{width:100%;border-collapse:collapse;margin:20px 0}th,td{text-align:left;padding:10px;border-bottom:1px solid #444}
button{padding:12px;font:inherit;cursor:pointer}#status{font-weight:bold}dt{color:#aaa;margin-top:10px}dd{margin:4px 0;font-variant-numeric:tabular-nums}
</style>
</head>
<body>
<h1>HX711 diagnostics</h1>
<p><a href="/">Race scales</a> · <a href="/settings">Scale settings</a> · <a href="/update">Firmware update</a></p>
<p id="device">Connecting…</p>
<p id="status" role="status"></p>
<p id="live_status" role="status">Connecting to live updates…</p>
<p>Recent conversions indicate ADC activity, but cannot confirm load-cell wiring.
If data is missing, check 3.3V, GND, DT and SCK. An unused channel B may float.</p>
<dl>
<dt>Firmware build / active OTA slot</dt><dd id="firmware">—</dd>
<dt>CPU clock</dt><dd id="cpu">—</dd>
<dt>Wiring / DT level</dt><dd id="pins">—</dd>
<dt>Conversions since boot / last conversion age / rate</dt><dd id="conversions">—</dd>
<dt>Normal weighing</dt><dd id="weighing">—</dd>
</dl>
<table>
<thead><tr><th>Channel</th><th>Raw ADC counts</th><th>Age</th><th>Samples</th><th>Status</th></tr></thead>
<tbody id="channels"></tbody>
</table>
<p>Raw values are signed 24-bit counts before tare, calibration, averaging or filtering.
Old values remain visible with their age. Saturation can indicate an overloaded or unwired input.</p>
<button id="toggle" type="button">Start A+B test (pauses this scale)</button>
<p>The A+B test alternates A at gain 128 and B at gain 32. Normal weighing and
tare/calibration on this scale pause during the test. Stop it to resume weighing.
It expires within 10 seconds if this page stops renewing it.</p>
<p id="error" role="alert"></p>
<dl>
<dt>Channel A: normal 10-sample raw average</dt><dd id="average">—</dd>
<dt>Stored tare offset / calibration (counts per lb)</dt><dd id="calibration">—</dd>
</dl>
<script src="/live.js"></script>
<script>
const el=id=>document.getElementById(id);
let testing=false, active=false;
async function mode(enabled){
  const r=await fetch('/diagnostics/mode',{method:'POST',body:new URLSearchParams({dual:enabled?'1':'0'})});
  const d=await r.json();
  if(!r.ok) throw Error(d.error || 'Cannot change diagnostic mode');
}
el('toggle').onclick=async()=>{
  el('toggle').disabled=true;
  try{const enabled=!active;await mode(enabled);testing=enabled;active=enabled;el('error').textContent=''}
  catch(e){el('error').textContent=e.message}
  finally{el('toggle').disabled=false}
};
function renderDiagnostics(d){
    active=d.dual;
    el('device').textContent='This board: '+d.id;
    el('cpu').textContent=d.cpu_mhz+' MHz';
    el('firmware').textContent='v'+(d.firmware_version || 'unknown')+' / '+d.firmware_build+' / '+d.app_partition;
    const suspect=d.rate_hz>100;
    el('status').textContent=suspect?'DT may be stuck LOW — conversion rate exceeds the HX711 maximum':
      d.responding?'Receiving HX711 conversions':'No recent HX711 data — check connection and power';
    el('status').style.color=d.responding && !suspect?'#4ade80':'#fbbf24';
    el('pins').textContent='DT: GPIO '+d.dt_pin+' / SCK: GPIO '+d.sck_pin+' / DT '+(d.dt_level?'HIGH (waiting)':'LOW (ready)');
    el('conversions').textContent=d.conversions+' / '+(d.last_conversion_ms===null?'never':d.last_conversion_ms+' ms')+' / '+d.rate_hz+' Hz';
    el('weighing').textContent=d.dual?'Paused for A+B diagnostics':d.weighing_ready?'Ready':'Waiting for 10 settled A samples';
    el('toggle').textContent=d.dual?'Stop A+B test / resume weighing':'Start A+B test (pauses this scale)';
    el('channels').replaceChildren();
    for(const c of d.channels){
      const tr=document.createElement('tr');
      const status=c.raw===null?'Not sampled':c.age_ms>=2000?'Stale':c.saturated?'SATURATED':'Live';
      for(const value of [c.name+' / gain '+c.gain,c.raw===null?'—':c.raw,c.age_ms===null?'—':c.age_ms+' ms',c.samples,status]){
        const td=document.createElement('td');td.textContent=value;tr.appendChild(td);
      }
      el('channels').appendChild(tr);
    }
    el('average').textContent=d.average_a===null?'Unavailable while paused, settling or disconnected':d.average_a;
    el('calibration').textContent=d.offset+' / '+d.calibration;
}
const live=connectLive('/diagnostics',renderDiagnostics,message=>{el('live_status').textContent=message});
setInterval(()=>{if(testing)live.send('diagnostics-active')},1000);
window.addEventListener('pagehide',()=>{
  if(testing)fetch('/diagnostics/mode',{method:'POST',body:new URLSearchParams({dual:'0'}),keepalive:true}).catch(()=>{});
});
</script>
</body>
</html>
)html";
