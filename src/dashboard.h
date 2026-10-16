#pragma once
#include <Arduino.h>
const char DASHBOARD[] PROGMEM = R"rawliteral(

  <!DOCTYPE html>
  <html>
  <head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">

  <style>

  body{
    margin:0;
    font-family:Arial;
    color:white;
    text-align:center;

  background:
    repeating-linear-gradient(
      45deg,
      #111,
      #111 6px,
      #0c0c0c 6px,
      #0c0c0c 12px
    );
  }

  .header{
    background:#16a34a;
    padding:8px;
    font-weight:bold;
  }

  .cararea{
    position:relative;
    width:90vw;        /* scales with screen width */
    max-width:420px;   /* prevents it getting huge on tablets */
    aspect-ratio: 8 / 7;
    margin:auto;
  }

  .vline{
    position:absolute;
    top:0;
    bottom:0;
    left:50%;
    width:2px;
    background:#16a34a;
  }

  .hline{
    position:absolute;
    left:0;
    right:0;
    top:50%;
    height:2px;
    background:#16a34a;
  }

  .weight{
    padding-top:8px;
    display:flex;
    flex-direction:column;
    align-items:center;
    justify-content:center;
    position:absolute;
    padding-bottom:8px;
  }

  .big{
    font-size:28px;
    font-weight:bold;
    font-family: 'Roboto Mono', monospace;
    font-variant-numeric: tabular-nums;
    letter-spacing:1px; 
    text-align:right;
    display:block;
    margin-bottom:4px;
    width:80px;
  }

  .small{
    font-size:14px;
    color:#22c55e;
    display:flex;
    flex-direction:column;
    align-items:center;
    justify-content:center;
    line-height:1.2;
    gap:2px;
  }

  .lf{ top:10px; left:-1%; }
  .rf{ top:10px; right:5%; }
  .lr{ bottom:10px; left:-1%; }
  .rr{ bottom:10px; right:5%; }

  .car{
    position:absolute;
    top:50%;
    left:50%;
    transform:translate(-50%,-50%);
  }

  .top{
    display:flex;
    align-items:center;
    gap:4px;
    font-size:14px;
    color:#22c55e;
    min-width:40px;
    width:auto;
  }

  .corner-name{
    display:inline-flex;
    flex-direction:column;
    align-items:center;
    gap:3px;
  }

  .corner-color{
    width:28px;
    height:12px;
    border:1px solid #888;
    border-radius:2px;
    box-sizing:border-box;
  }

  .lf .corner-color{ background:#ef4444; }
  .rf .corner-color{ background:#22c55e; }
  .lr .corner-color{ background:#fff; }
  .rr .corner-color{ background:#000; }
  .battery{ font-size:12px; color:#ccc; white-space:nowrap; }

  .value{
    display:flex;
    align-items:center;
    justify-content:center;
    gap:6px;
    min-height:34px;
    position:relative;
    margin-top: 76px;
  }

  .panel{
    margin:10px;
    border:2px solid #16a34a;
    border-radius:10px;
    padding:12px;
    background:#0e0e0e;
  }

  /* table */

  .stats{
    width:100%;
    font-size:18px;
    border-collapse:collapse;
    table-layout:fixed; 
  }

  .stats th{
    font-size:14px;
    opacity:.8;
    padding-bottom:6px;
  }

  .stats td{
    padding:6px 4px;
  }

  .stats th:nth-child(1),
  .stats td:nth-child(1){
    width:30%;
  }

  .stats th:nth-child(2),
  .stats td:nth-child(2){
    width:35%;
  }

  .stats th:nth-child(3),
  .stats td:nth-child(3){
    width:35%;
  }

  #frontpct,
  #rearpct,
  #leftpct,
  #rightpct,
  #crosspct{
    font-variant-numeric: tabular-nums;
    text-align:right;
    display:inline-block;
    width:60px;   /* fixed width prevents shift */
  }

  .val{
    color:#22c55e;
    font-weight:bold;
    font-variant-numeric: tabular-nums;
    text-align:right;
  }

  .totalrow td{
    font-size:20px;
    font-weight:bold;
    border-top:1px solid #333;
    padding-top:10px;
  }

  /* buttons */

  .buttons{
    margin-top:10px;
    display:flex;
    justify-content:center;
    gap:12px;
  }

  button{
    font-size:18px;
    padding:12px 24px;
    border-radius:8px;
    border:none;
    cursor:pointer;
  }

  .zero{
    background:#16a34a;
    color:white;
  }

  .cal{
    background:#333;
    color:white;
  }

  .status{
    display:inline-block;
    width:10px;
    height:10px;
    border-radius:50%;
    margin-left:5px;
    background:#444;
  }

  .online{
    background:#22c55e;
  }

  .offline{
    background:#ef4444;
  }

  .lock{
    width:18px;
    display:inline-block;
    text-align:left;
    opacity:0.3;
    line-height:1;
    font-size:14px;
  }

  .locked{
    opacity:1;
    color:#22c55e;
  }
  .digits{
    display:flex;
    justify-content:center;
    align-items:center;
    gap:2px;
    height:32px;
  }

  .digit{
    width:16px;
    text-align:center;
    font-size:24px;
    font-weight:bold;
    font-family: 'Roboto Mono', monospace;
  }

  .decimal{
    width:16px;
  }

  .lf .top,
  .lr .top{
    left:6px;
    right:auto;
    justify-content:flex-start;
    flex-direction:row-reverse;
  }

  /* RIGHT side */
  .rf .top,
  .rr .top{
    right:6px;
    left:auto;
    justify-content:flex-end;
  }

  .lf .top,
  .rf .top{
    position:absolute;
    top:4px;
  }

  .lr .top,
  .rr .top{
    position:absolute;
    bottom:4px;
    top:auto;
  }

  .lr .value,
  .rr .value{
    margin-top:0px;
    margin-bottom:76px;
  }

  </style>

  </head>

  <body>

  <div class="header">
  DIY Corner Weight System
  </div>

  <div class="cararea">

  <div class="vline"></div>
  <div class="hline"></div>

  <div class="weight lf">
    <div class="top">
      <span class="corner-name" title="Red scale">LF<span class="corner-color"></span><span class="battery" id="lf_batt">Battery: --</span><span class="battery" id="lf_pct" title="Estimated usable charge: 3.64 V empty, 4.20 V full">--</span></span>
      <span class="status" id="lf_status"></span>
    </div>
    <div class="value">
      <div class="digits" id="lf"></div>
      <span id="lf_lock" class="lock"></span>
    </div>
  </div>

  <div class="weight rf">
    <div class="top">
      <span class="corner-name" title="Green scale">RF<span class="corner-color"></span><span class="battery" id="rf_batt">Battery: --</span><span class="battery" id="rf_pct" title="Estimated usable charge: 3.64 V empty, 4.20 V full">--</span></span>
      <span class="status" id="rf_status"></span>
      </div>
      <div class="value">
        <div class="digits" id="rf"></div>
        <span id="rf_lock" class="lock"></span>
      </div>
  </div>

  <div class="weight lr">
    <div class="top">
      <span class="corner-name" title="White scale">LR<span class="corner-color"></span><span class="battery" id="lr_batt">Battery: --</span><span class="battery" id="lr_pct" title="Estimated usable charge: 3.64 V empty, 4.20 V full">--</span></span>
      <span class="status" id="lr_status"></span>
      </div>
      <div class="value">
        <div class="digits" id="lr"></div>
        <span id="lr_lock" class="lock"></span> 
    </div>
  </div>

  <div class="weight rr">
    <div class="top">
      <span class="corner-name" title="Black scale">RR<span class="corner-color"></span><span class="battery" id="rr_batt">Battery: --</span><span class="battery" id="rr_pct" title="Estimated usable charge: 3.64 V empty, 4.20 V full">--</span></span>
      <span class="status" id="rr_status"></span>
      </div>
      <div class="value">
        <div class="digits" id="rr"></div>
        <span id="rr_lock" class="lock"></span> 
    </div>
  </div>

  <div class="car">

  <svg width="120" height="220" viewBox="0 0 220 300">

  <path fill="#16a34a" stroke="#111" stroke-width="3" d="
  M85 10
  L135 10
  Q155 20 160 50
  L165 100
  L165 200
  L160 250
  Q155 280 135 290
  L85 290
  Q65 280 60 250
  L55 200
  L55 100
  L60 50
  Q65 20 85 10
  Z"/>

  <rect x="75" y="55" width="70" height="40" rx="10" fill="#2f2f2f"/>
  <rect x="75" y="215" width="70" height="40" rx="10" fill="#2f2f2f"/>

  <polygon points="55,120 45,130 55,140" fill="#16a34a"/>
  <polygon points="165,120 175,130 165,140" fill="#16a34a"/>

  </svg>

  </div>

  </div>

  <div class="panel">
    <table class="stats">
    <thead>
      <tr>
        <th></th>
        <th>Current</th>
        <th>%</th>
      </tr>
    </thead>
    <tbody>
      <tr>
        <td>Cross</td>
        <td>-</td>
        <td id="crosspct"></td>
      </tr>
      <tr>
        <td>Left</td>
        <td class="val" id="left"></td>
        <td id="leftpct"></td>
      </tr>
      <tr>
        <td>Right</td>
        <td class="val" id="right"></td>
        <td id="rightpct"></td>
      </tr>
      <tr>
        <td>Front</td>
        <td class="val" id="front"></td>
        <td id="frontpct"></td>
      </tr>
      <tr>
      <td>Rear</td>
        <td class="val" id="rear"></td>
        <td id="rearpct"></td>
      </tr>
      <tr class="totalrow">
        <td>Total</td>
        <td class="val" id="total"></td>
      <td></td>
      </tr>
    </tbody>
    </table>
  </div>
  <div class="buttons">
    <button class="zero" onclick="tare()">ZERO</button>
    <button class="cal" onclick="calibrate()">CAL</button>
    <button onclick="toggleUnits()" id="unitBtn">LBS</button>
  </div>
  <p id="live_status" role="status">Connecting to live updates…</p>
  <p id="kit_status"></p>
  <div class="panel" style="overflow-x:auto"><table style="width:100%;font-size:14px">
    <thead><tr><th>Scale</th><th>Firmware</th><th>Corner</th><th>Weight</th><th>Gain (counts/lb)</th><th>Zero offset (lb)</th><th>Status</th></tr></thead>
    <tbody id="node_list"></tbody>
  </table></div>
  <div class="panel" id="led_guide" style="font-size:14px">
    <strong>LED guide</strong>
    <p><span style="color:#22c55e">Green</span>: Child/member ·
       <span style="color:#60a5fa">Blue</span>: Parent/gateway connected to Wi-Fi ·
       <span style="color:#facc15">Yellow</span>: Parent/gateway hosting its AP without a Wi-Fi association</p>
    <p><span style="color:#fff">White</span>: Charging/USB power (above 4.5 V) ·
       <span style="color:#ef4444">Red</span>: Low battery (under 20%). Battery pulse off otherwise.</p>
  </div>
  <p id="page_links"><a href="/settings" style="color:#22c55e">Scale settings</a> ·
     <a href="/wifi" style="color:#22c55e">Wi-Fi setup</a> ·
     <a href="/diagnostics" style="color:#22c55e">HX711 diagnostics</a></p>
  <script src="/live.js"></script>
  <script>
  let useKg = false;
  let latestData;

  async function command(url){
    let r = await fetch(url, {method:'POST'});
    let d = await r.json();
    if(!r.ok) throw Error(d.error);
    for(let i=0;i<30;i++){
      await new Promise(resolve=>setTimeout(resolve,250));
      r=await fetch('/operation?id='+d.operation);
      let result=await r.json();
      if(!r.ok) throw Error(result.error);
      if(!result.pending){
        let errors=result.results.filter(x=>!x.ok);
        if(errors.length) throw Error(errors.map(x=>x.id+': '+x.message).join('\n'));
        return;
      }
    }
    throw Error('The scale did not confirm the change');
  }

  function calibrate(){

    let pad = prompt("Corner (LF RF LR RR) or scale ID. Leave blank when using one scale.");
    if(pad === null) return;

    let weight = parseFloat(prompt(
      "Known weight (" + (useKg ? "KG" : "LBS") + ")"
    ));

    if(isNaN(weight)) return;

    // convert KG -> LBS before sending to ESP32
    if(useKg){
      weight = weight * 2.20462;
    }

    command("/calibrate?pad=" + encodeURIComponent(pad.trim()) + "&weight=" + weight)
      .catch(e=>alert(e.message));
  }

  function format(val){
    return val.toFixed(1);
  }

  function convert(val){
    let v = useKg ? val * 0.453592 : val;
    return v;
  }

  function toggleUnits(){
    useKg = !useKg;
    document.getElementById("unitBtn").innerText = useKg ? "KG" : "LBS";
    if(latestData)renderData(latestData);
  }

  function renderDigits(id, value){
    let el = document.getElementById(id)
    let str = value === null ? '--' : useKg
      ? value.toFixed(1)
      : (Math.round(value * 2) / 2).toFixed(1)
    str = str.padStart(6, " ")
    el.innerHTML = ""
    for(let c of str){
      let d = document.createElement("span")
      d.className = "digit"
      if(c === "."){
        d.classList.add("decimal")
      }
      d.innerText = c
      el.appendChild(d)
    }
  }

  function renderData(d){
      latestData=d;
      renderDigits("lf", d.lf_online ? convert(d.lf) : null)
      renderDigits("rf", d.rf_online ? convert(d.rf) : null)
      renderDigits("lr", d.lr_online ? convert(d.lr) : null)
      renderDigits("rr", d.rr_online ? convert(d.rr) : null)

      total.innerText = format(convert(d.total))
      front.innerText = format(convert(d.front))
      rear.innerText = format(convert(d.rear))
      left.innerText = format(convert(d.left))
      right.innerText = format(convert(d.right))
      frontpct.innerText=d.balance_available ? d.frontpct : '—'
      rearpct.innerText=d.balance_available ? d.rearpct : '—'
      leftpct.innerText=d.balance_available ? d.leftpct : '—'
      rightpct.innerText=d.balance_available ? d.rightpct : '—'
      crosspct.innerText=d.balance_available ? d.cross : '—'
      kit_status.textContent='Kit '+d.kit+' · Gateway v'+(d.firmware_version || 'unknown')+' · '+d.online_count+' scale(s) connected · '+
        (d.balance_available ? 'Corner balance ready' : 'Corner balance needs four calibrated, assigned scales')
      node_list.replaceChildren()
      const corners=['Unassigned','LF','RF','LR','RR']
      const seen=new Set(), duplicates=new Set()
      d.nodes.filter(n=>n.online && n.corner).forEach(n=>{
        if(seen.has(n.corner)) duplicates.add(n.corner)
        seen.add(n.corner)
      })
      for(let c=1;c<=4;c++){
        const matches=d.nodes.filter(n=>n.online && n.corner===c)
        renderVoltages(corners[c].toLowerCase()+'_batt',matches.length===1 ? matches[0] : null)
      }
      d.nodes.forEach(n=>{
        let tr=document.createElement('tr')
        let status=!n.online ? 'Offline' : !n.ready ? 'No load-cell reading' :
          !n.calibrated ? 'Needs calibration' : duplicates.has(n.corner) ? 'Corner assigned twice' : 'Ready'
        for(let value of [n.id+(n.wifi_host ? ' (WiFi)' : ''),n.firmware_version || 'unknown',corners[n.corner],n.online && n.ready && n.calibrated ?
            format(convert(n.weight))+(useKg ? ' kg' : ' lb') : '--',
            Number.isFinite(n.calibration) ? n.calibration.toFixed(6) : '—',
            Number.isFinite(n.offset) && Number.isFinite(n.calibration) && Math.abs(n.calibration)>0.0001
              ? (n.offset/n.calibration).toFixed(1) : '—',status]){
          let td=document.createElement('td');td.textContent=value;tr.appendChild(td)
        }
        node_list.appendChild(tr)
      })
      
      setStatus("lf_status",d.lf_online)
      setStatus("rf_status",d.rf_online)
      setStatus("lr_status",d.lr_online)
      setStatus("rr_status",d.rr_online)

      setLock("lf_lock", d.lf_locked)
      setLock("rf_lock", d.rf_locked)
      setLock("lr_lock", d.lr_locked)
      setLock("rr_lock", d.rr_locked)

  }

  function batteryPercent(voltage){
    if(!Number.isFinite(voltage))return null
    // Approximate Li-ion discharge shape, rescaled to this kit's usable voltage window.
    const curve=[[3.64,0],[3.80,20],[3.90,50],[4.00,75],[4.10,90],[4.20,100]]
    if(voltage<=curve[0][0])return 0
    for(let i=1;i<curve.length;i++){
      const [v0,p0]=curve[i-1], [v1,p1]=curve[i]
      if(voltage<=v1)return Math.round(p0+(voltage-v0)*(p1-p0)/(v1-v0))
    }
    return 100
  }

  function renderVoltages(id,node){
    const voltage=node?.battery_voltage,percent=batteryPercent(voltage)
    const volts=value=>Number.isFinite(value) ? value.toFixed(2)+' V' : '--'
    document.getElementById(id).textContent='Battery: '+volts(voltage)
    document.getElementById(id.replace('_batt','_pct')).textContent=
      percent===null ? '--' : voltage>4.5 ? 'USB power' : '~'+percent+'%'
  }

  function setStatus(id,online){
    let el=document.getElementById(id)
    if(online){
      el.className="status online"
    } else {
      el.className="status offline"
    }
  }

  function tare(){
    command("/tare").catch(e=>alert(e.message))
  }
  
  function setLock(id, locked){
    let el = document.getElementById(id)

    if(locked){
      el.innerText = "🔒"
      el.className = "lock locked"
    } else {
      el.innerText = ""
      el.className = "lock"
    }
  }

  connectLive('/scales',renderData,message=>{
    document.getElementById('live_status').textContent=message;
    if(message !== 'Live updates'){
      latestData=null;
      for(const corner of ['lf','rf','lr','rr']){
        renderDigits(corner,null);
        setStatus(corner+'_status',false);
        setLock(corner+'_lock',false);
        renderVoltages(corner+'_batt',null);
      }
      for(const id of ['total','front','rear','left','right','frontpct','rearpct','leftpct','rightpct','crosspct']){
        document.getElementById(id).innerText='--';
      }
    }
  });

  </script>

  </body>
  </html>

  )rawliteral";
