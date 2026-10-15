#pragma once

const char KIT_UPDATE[] PROGMEM = R"html(
<section><h2>Update all modules</h2>
<p>Select one ESP32-S3 application .bin for this kit. Members update one at a time;
the gateway updates last. Keep this page open and all modules powered until finished.</p>
<p>Scale-only modules are skipped. Set a module to Auto or Prefer gateway to include it.</p>
<form id="kit_upload"><input id="kit_file" type="file" accept=".bin" required>
<button id="kit_submit">Update all modules</button></form>
<progress id="kit_progress" max="100" value="0" style="width:100%"></progress>
<p id="kit_status" role="status"></p><ol id="kit_log"></ol></section>
<script>
document.getElementById('kit_upload').onsubmit=async e=>{
  e.preventDefault();
  const file=document.getElementById('kit_file').files[0];
  if(!file)return;
  const button=document.getElementById('kit_submit'),status=document.getElementById('kit_status');
  if(button.disabled)return;
  button.disabled=true;
  const log=document.getElementById('kit_log');log.replaceChildren();
  const record=text=>{const row=document.createElement('li');row.textContent=text;log.appendChild(row)};
  const sleep=()=>new Promise(resolve=>setTimeout(resolve,1000));
  const json=async url=>{
    const response=await fetch(url,{cache:'no-store',signal:AbortSignal.timeout(5000)});
    if(!response.ok)throw Error('Cannot read scale status ('+response.status+')');
    return response.json();
  };
  const upload=(node,gateway)=>new Promise((resolve,reject)=>{
    const request=new XMLHttpRequest();
    request.open('POST',gateway ? '/u' : '/member-update/'+node.id);
    request.timeout=180000;
    request.upload.onprogress=e=>{
      if(e.lengthComputable)document.getElementById('kit_progress').value=100*e.loaded/e.total;
    };
    request.onload=()=>{
      try{
        if(gateway){
          if(request.status!==200 || !request.responseText.includes('Update successful.'))
            throw Error('Gateway did not accept the firmware. Check its version before retrying.');
        }else{
          const result=JSON.parse(request.responseText);
          if(request.status!==200 || !result.ok)throw Error(result.message || 'Upload failed');
        }
        resolve();
      }catch(error){reject(error)}
    };
    request.onerror=request.ontimeout=()=>reject(Error('Connection lost or timed out; check '+node.id+' before retrying.'));
    if(gateway){
      const form=new FormData();form.append('update',file,file.name);request.send(form);
    }else{
      request.setRequestHeader('Content-Type','application/octet-stream');request.send(file);
    }
  });
  try{
    status.textContent='Checking modules…';
    document.getElementById('kit_progress').value=0;
    const magic=new Uint8Array(await file.slice(0,1).arrayBuffer())[0];
    if(file.size<24 || magic!==0xe9)throw Error('Choose an ESP32-S3 application .bin file.');
    const data=await json('/data');
    if(data.role!=='gateway')throw Error('Open Scale settings on the current gateway.');
    const gateway=data.nodes.find(n=>n.wifi_host);
    if(!gateway)throw Error('Gateway scale unavailable. Refresh Scale settings.');
    const skipped=data.nodes.filter(n=>n.preference===2);
    skipped.forEach(n=>record(n.id+': skipped — firmware updates are not available in Scale only mode.'));
    const targets=[...data.nodes.filter(n=>n.id!==gateway.id && n.preference!==2),gateway];
    for(const node of targets){
      if(!node.online)throw Error(node.id+' is offline. Reconnect all modules before updating.');
      if(!node.update_available)throw Error(node.id+' needs a gateway Wi-Fi connection for OTA; ESP-NOW alone is insufficient.');
    }
    for(let index=0;index<targets.length;index++){
      const node=targets[index],isGateway=node.id===gateway.id;
      status.textContent='Uploading '+(index+1)+'/'+targets.length+': '+node.id+(isGateway ? ' (gateway)' : '');
      document.getElementById('kit_progress').value=0;
      try{await upload(node,isGateway)}catch(error){throw Error(node.id+': '+error.message)}
      record(node.id+': firmware accepted; waiting for reboot…');
      status.textContent='Waiting for '+node.id+' to restart…';
      let restarted=null;
      for(let attempt=0;attempt<90;attempt++){
        await sleep();
        try{
          if(isGateway){
            const current=await json('/diagnostics/data');
            if(current.id===node.id && Number.isInteger(current.boot_id) && current.boot_id!==node.boot_id){restarted=current;break}
          }else{
            const current=await json('/data');
            if(current.role!=='gateway')throw Error('Gateway changed');
            restarted=current.nodes.find(n=>n.id===node.id && n.online && Number.isInteger(n.boot_id) && n.boot_id!==node.boot_id);
            if(restarted)break;
          }
        }catch(error){}
      }
      if(!restarted)throw Error(node.id+': firmware accepted, but reboot/reconnection was not confirmed. Reconnect to the gateway and check versions. Remaining uploads stopped.');
      record(node.id+': restarted, firmware '+(restarted.firmware_version || 'unknown'));
    }
    status.textContent='All '+targets.length+' eligible modules updated and restarted.'+
      (skipped.length ? ' '+skipped.length+' Scale-only module(s) skipped.' : '');
  }catch(error){status.textContent=error.message}
  finally{button.disabled=false}
};
</script>
)html";
