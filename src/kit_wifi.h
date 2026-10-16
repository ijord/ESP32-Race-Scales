#pragma once

const char KIT_WIFI[] PROGMEM = R"html(
<section><h2>Share Wi-Fi with all scales</h2>
<p>Save the same home-network credentials on every scale, including Scale-only modules.
Members stay on the kit Wi-Fi; saved credentials are used when a scale becomes gateway.
All modules must run firmware 1.2.11 or newer and have a kit Wi-Fi connection.</p>
<form id="kit_wifi">
<label>Credentials <select id="kit_wifi_source">
<option value="saved" %SAVED_OPTION%>Use gateway's saved home Wi-Fi</option>
<option value="manual">Enter a network</option></select></label>
<fieldset id="kit_wifi_manual" hidden disabled>
<label>SSID <input id="kit_wifi_ssid" maxlength="32" required autocomplete="off"></label>
<label>Password <input id="kit_wifi_password" type="password" minlength="8" maxlength="64" autocomplete="new-password"></label>
<p>Leave the password blank only for an open network.</p></fieldset>
<button id="kit_wifi_submit">Save Wi-Fi on all scales</button></form>
<p id="kit_wifi_status" role="status"></p><ol id="kit_wifi_log"></ol></section>
<script>
const wifiSource=document.getElementById('kit_wifi_source');
wifiSource.onchange=()=>{
  const manual=document.getElementById('kit_wifi_manual');
  manual.hidden=manual.disabled=wifiSource.value!=='manual';
};
wifiSource.onchange();
document.getElementById('kit_wifi').onsubmit=async e=>{
  e.preventDefault();
  const button=document.getElementById('kit_wifi_submit'),status=document.getElementById('kit_wifi_status');
  if(button.disabled)return;
  button.disabled=true;
  const log=document.getElementById('kit_wifi_log');log.replaceChildren();
  const record=text=>{const row=document.createElement('li');row.textContent=text;log.appendChild(row)};
  const parameters=new URLSearchParams({source:wifiSource.value});
  if(wifiSource.value==='manual'){
    parameters.set('ssid',document.getElementById('kit_wifi_ssid').value);
    parameters.set('password',document.getElementById('kit_wifi_password').value);
  }
  try{
    status.textContent='Checking scales…';
    const response=await fetch('/data',{cache:'no-store',signal:AbortSignal.timeout(5000)});
    if(!response.ok)throw Error('Cannot read scale status.');
    const data=await response.json(),gateway=data.nodes.find(n=>n.wifi_host);
    if(data.role!=='gateway' || !gateway)throw Error('Open Scale settings on the current gateway.');
    const targets=[...data.nodes.filter(n=>n.id!==gateway.id),gateway];
    let saved=0;
    for(const node of targets){
      status.textContent='Saving Wi-Fi on '+node.id+'…';
      if(!node.online){record(node.id+': offline — not saved.');continue}
      try{
        const body=new URLSearchParams(parameters);body.set('id',node.id);
        const result=await fetch('/kit-wifi',{method:'POST',body,signal:AbortSignal.timeout(12000)});
        const reply=await result.json();
        if(!result.ok || !reply.ok)throw Error(reply.error || 'Credentials were not saved.');
        saved++;record(node.id+': saved.');
      }catch(error){record(node.id+': '+error.message)}
    }
    status.textContent='Wi-Fi saved on '+saved+' of '+targets.length+' scales.'+
      (saved===targets.length ? ' Current kit connections are unchanged.' : ' Reconnect or update the failed scales, then retry.');
  }catch(error){status.textContent=error.message}
  finally{document.getElementById('kit_wifi_password').value='';button.disabled=false}
};
</script>
)html";
