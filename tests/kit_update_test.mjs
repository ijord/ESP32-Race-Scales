// Exercise the production kit updater with deterministic Wi-Fi/reboot responses.
import {readFile} from 'node:fs/promises';
import assert from 'node:assert/strict';
import vm from 'node:vm';

const html=await readFile(new URL('../src/kit_update.h',import.meta.url),'utf8');
const script=html.match(/<script>([\s\S]*?)<\/script>/)[1];
class Element {
  children=[];textContent='';disabled=false;
  replaceChildren(){this.children=[]}
  appendChild(child){this.children.push(child)}
}
async function run(options={}){
  const elements=Object.fromEntries(['kit_upload','kit_file','kit_submit','kit_status','kit_log','kit_progress']
    .map(id=>[id,new Element()]));
  const file={name:'firmware.bin',size:100,slice:()=>({arrayBuffer:async()=>new Uint8Array([options.badImage ? 0 : 0xe9]).buffer})};
  elements.kit_file.files=[file];
  const targets=[
    {id:'HOST',online:true,update_available:true,wifi_host:true,boot_id:10},
    {id:'MEMBER',online:!options.offline,update_available:!options.noWifi,boot_id:20,preference:0},
    {id:'OTHER',online:true,update_available:true,boot_id:30}
  ];
  if(options.scaleOnly)targets.push({id:'SCALE_ONLY',online:false,update_available:false,boot_id:40,preference:2});
  const requests=[],completed=new Set();
  let pollCount=0;
  class FormData { entries=[];append(...args){this.entries.push(args)} }
  class Request {
    upload={};headers={};
    open(method,path){this.method=method;this.path=path}
    setRequestHeader(name,value){this.headers[name]=value}
    send(body){
      requests.push(this);this.body=body;
      queueMicrotask(()=>{
        if(options.networkFailure){this.onerror();return}
        if(options.timeout){this.ontimeout();return}
        const gateway=this.path==='/u',id=gateway ? 'HOST' : this.path.split('/').pop();
        const fail=options.fail===id;
        this.status=fail ? 502 : 200;
        this.responseText=gateway ? (fail ? 'Update failed!' : 'Update successful.') :
          JSON.stringify(fail ? {ok:false,message:'Relay failed'} : {ok:true});
        if(!fail)completed.add(id);
        this.upload.onprogress({lengthComputable:true,loaded:100,total:100});
        this.onload();
      });
    }
  }
  const fetch=async url=>{
    pollCount++;
    if(url==='/diagnostics/data')return {ok:true,json:async()=>({id:'HOST',boot_id:options.missingBoot ? undefined : 11,firmware_version:'1.2.6'})};
    return {ok:true,json:async()=>({role:options.wrongRole ? 'member' : 'gateway',nodes:targets.map(node=>
      ({...node,boot_id:completed.has(node.id) && !options.noReboot ? node.boot_id+1 : node.boot_id,firmware_version:'1.2.6'}))})};
  };
  vm.runInNewContext(script,{
    document:{getElementById:id=>elements[id],createElement:()=>new Element()},
    XMLHttpRequest:Request,FormData,Uint8Array,fetch,AbortSignal:{timeout:()=>undefined},
    setTimeout:callback=>queueMicrotask(callback)
  });
  await elements.kit_upload.onsubmit({preventDefault(){}});
  return {elements,file,requests,pollCount};
}
const success=await run();
assert.deepEqual(success.requests.map(r=>r.path),['/member-update/MEMBER','/member-update/OTHER','/u']);
assert.equal(success.requests[0].body,success.file);
assert.equal(success.requests[1].body,success.file);
assert.equal(success.requests[0].headers['Content-Type'],'application/octet-stream');
assert.equal(success.requests[2].body.entries[0][0],'update');
assert.equal(success.requests[2].body.entries[0][1],success.file);
assert.match(success.elements.kit_status.textContent,/All 3 eligible modules updated and restarted/);
assert.equal(success.elements.kit_log.children.length,6);
assert.equal(success.elements.kit_submit.disabled,false);
assert.equal(success.elements.kit_progress.value,100);

for(const option of ['offline','noWifi','badImage','wrongRole']){
  const result=await run({[option]:true});
  assert.equal(result.requests.length,0,option+' must stop before flashing');
  assert.equal(result.elements.kit_submit.disabled,false);
}
for(const options of [{fail:'MEMBER'},{networkFailure:true},{timeout:true},{noReboot:true}]){
  const result=await run(options);
  assert.equal(result.requests.length,1,'Member failure must stop before the next member/gateway');
  assert.doesNotMatch(result.elements.kit_status.textContent,/All 3 eligible modules updated/);
  assert.equal(result.elements.kit_log.children.filter(row=>row.textContent===result.elements.kit_status.textContent).length,0,
    'Failures should appear once in the status, not repeated in the log');
}
const skipped=await run({scaleOnly:true});
assert.deepEqual(skipped.requests.map(r=>r.path),success.requests.map(r=>r.path));
assert.match(skipped.elements.kit_status.textContent,/1 Scale-only module\(s\) skipped/);
assert.match(skipped.elements.kit_log.children[0].textContent,/SCALE_ONLY: skipped/);
const gatewayFailure=await run({fail:'HOST'});
assert.match(gatewayFailure.elements.kit_status.textContent,/Gateway did not accept/);
const noGatewayBoot=await run({missingBoot:true});
assert.match(noGatewayBoot.elements.kit_status.textContent,/reboot\/reconnection was not confirmed/);
assert.ok(noGatewayBoot.pollCount>=90); // Missing boot ID cannot count as a verified reboot.
console.log('PASS: one-file kit OTA, Scale-only skip, gateway last, reboot checks, preflight and failure stop');
