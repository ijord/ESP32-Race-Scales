#pragma once

const char MEMBER_UPDATE[] PROGMEM = R"html(
<!doctype html><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Update scale firmware</title>
<style>body{font:16px Arial;background:#111;color:#eee;max-width:640px;margin:30px auto;padding:15px}
a{color:#22c55e}input,button{font:inherit;padding:10px;margin:10px 0}progress{width:100%}</style>
<h1>Update scale firmware</h1>
<p><a href="/settings">Scale settings</a></p>
<p>Selected scale: <strong>%TARGET%</strong></p>
<p>The gateway will relay this upload to the selected scale. Stay on your current Wi-Fi.
Live readings may pause during transfer. Use the ESP32-S3 application firmware.bin file.</p>
<form id="upload"><input id="file" type="file" accept=".bin" required><br><button id="submit">Upload firmware</button></form>
<progress id="progress" max="100" value="0"></progress><p id="status" role="status"></p>
<script>
document.getElementById('upload').onsubmit=e=>{
  e.preventDefault();
  const file=document.getElementById('file').files[0];
  if(!file)return;
  const button=document.getElementById('submit'),status=document.getElementById('status');
  button.disabled=true;status.textContent='Uploading to the selected scale…';
  const request=new XMLHttpRequest();
  request.open('POST',location.pathname);
  request.setRequestHeader('Content-Type','application/octet-stream');
  request.timeout=180000;
  request.upload.onprogress=e=>{
    if(e.lengthComputable)document.getElementById('progress').value=100*e.loaded/e.total;
  };
  request.onload=()=>{
    try{
      const result=JSON.parse(request.responseText);
      if(request.status!==200 || !result.ok)throw Error(result.message || 'Upload failed');
      status.textContent='Firmware accepted. The selected scale is rebooting. Check its version in Scale settings or the live scale list.';
    }catch(error){status.textContent=error.message;button.disabled=false}
  };
  request.onerror=request.ontimeout=()=>{
    status.textContent='Connection lost or timed out. Check the selected scale’s version before retrying.';
    button.disabled=false;
  };
  request.send(file);
};
</script>
)html";
