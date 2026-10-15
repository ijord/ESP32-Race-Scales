// Exercise the actual browser connection helper with controlled sockets/timers.
import {readFile, writeFile, mkdtemp, unlink, rmdir} from 'node:fs/promises';
import assert from 'node:assert/strict';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {pathToFileURL} from 'node:url';

const source=await readFile(new URL('../src/live_updates.h',import.meta.url),'utf8');
const script=source.split('R"js(')[1].split(')js";')[0];
const output=await mkdtemp(join(tmpdir(),'race-scales-live-'));
const moduleUrl=pathToFileURL(join(output,'live_updates.mjs'));
let connectLive;
try {
  await writeFile(moduleUrl,script+'\nexport {connectLive};\n');
  ({connectLive}=await import(moduleUrl.href));
} finally {
  await unlink(moduleUrl);
  await rmdir(output);
}

const saved={window:globalThis.window,location:globalThis.location,WebSocket:globalThis.WebSocket,
  setInterval:globalThis.setInterval,clearInterval:globalThis.clearInterval,
  setTimeout:globalThis.setTimeout,clearTimeout:globalThis.clearTimeout,now:Date.now};
const intervals=new Map(),timeouts=new Map(),events={};
let timerId=0,now=0;
class FakeSocket {
  static OPEN=1;
  static instances=[];
  constructor(url){this.url=url;this.readyState=0;this.sent=[];FakeSocket.instances.push(this)}
  send(text){this.sent.push(text)}
  close(){this.readyState=3;if(!this.deferClose)this.onclose()}
}
try {
  globalThis.window={addEventListener:(type,callback)=>{events[type]=callback}};
  globalThis.location={hostname:'192.168.4.1'};
  globalThis.WebSocket=FakeSocket;
  globalThis.setInterval=callback=>{intervals.set(++timerId,callback);return timerId};
  globalThis.clearInterval=id=>intervals.delete(id);
  globalThis.setTimeout=(callback,delay)=>{assert.equal(delay,1000);timeouts.set(++timerId,callback);return timerId};
  globalThis.clearTimeout=id=>timeouts.delete(id);
  Date.now=()=>now;
  const received=[],statuses=[];
  const live=connectLive('/scales',data=>received.push(data),status=>statuses.push(status));
  const first=FakeSocket.instances[0];
  assert.equal(first.url,'ws://192.168.4.1:81/scales');
  assert.equal(live.send('test'),false);
  first.readyState=FakeSocket.OPEN;
  first.onmessage({data:'{"total":123}'});
  assert.deepEqual(received,[{total:123}]);
  assert.equal(statuses.at(-1),'Live updates');
  assert.equal(live.send('test'),true);
  assert.deepEqual(first.sent,['test']);

  now=4000;
  for(const callback of [...intervals.values()])callback();
  assert.equal(first.readyState,3);
  assert.match(statuses.at(-1),/stale/);
  assert.equal(timeouts.size,1);
  const retry=[...timeouts.values()][0];timeouts.clear();retry();
  assert.equal(FakeSocket.instances.length,2);
  first.onmessage({data:'{"total":999}'});
  assert.equal(received.length,1,'Old socket data must be ignored');

  // A delayed close event from a cached page must not reconnect a second time.
  const second=FakeSocket.instances[1];second.deferClose=true;
  events.pagehide();
  assert.equal(intervals.size,0);
  assert.equal(timeouts.size,0);
  events.pageshow({persisted:true});
  assert.equal(FakeSocket.instances.length,3);
  second.onclose();
  assert.equal(timeouts.size,0);
  assert.equal(intervals.size,1);
  const third=FakeSocket.instances[2];
  third.onmessage({data:'not JSON'});
  assert.equal(third.readyState,3);
  assert.equal(timeouts.size,1);
  events.pagehide();
  assert.equal(intervals.size,0);
  assert.equal(timeouts.size,0);
  console.log('PASS: live rendering, sends, stale timeout, reconnect, cached-page restore, malformed data and cleanup');
} finally {
  for(const key of ['window','location','WebSocket','setInterval','clearInterval','setTimeout','clearTimeout']) {
    if(saved[key]===undefined)delete globalThis[key];else globalThis[key]=saved[key];
  }
  Date.now=saved.now;
}
