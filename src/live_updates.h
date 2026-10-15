#pragma once
#include <Arduino.h>

const char LIVE_UPDATES[] PROGMEM = R"js(
// One persistent connection per page. Reconnect if data stops arriving.
function connectLive(path, onData, onStatus) {
  let socket, retry, watchdog, stopped=false;
  function open() {
    if(stopped)return;
    onStatus('Connecting to live updates…');
    const current=new WebSocket('ws://'+location.hostname+':81'+path);
    socket=current;
    let received=Date.now();
    const timer=setInterval(()=>{
      if(Date.now()-received>3000)current.close();
    },500);
    watchdog=timer;
    current.onmessage=event=>{
      if(stopped || socket!==current)return;
      try{
        onData(JSON.parse(event.data));
        received=Date.now();
        onStatus('Live updates');
      }catch(e){current.close()}
    };
    current.onerror=()=>current.close();
    current.onclose=()=>{
      clearInterval(timer);
      if(stopped || socket!==current)return;
      onStatus('Reconnecting — displayed readings may be stale');
      retry=setTimeout(open,1000);
    };
  }
  window.addEventListener('pagehide',()=>{
    stopped=true;
    clearTimeout(retry);
    clearInterval(watchdog);
    if(socket)socket.close();
  });
  window.addEventListener('pageshow',event=>{
    if(event.persisted){stopped=false;open()}
  });
  open();
  return {
    send(text){
      if(socket && socket.readyState===WebSocket.OPEN){socket.send(text);return true}
      return false;
    }
  };
}
)js";
