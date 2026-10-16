import {readFile} from 'node:fs/promises';
import assert from 'node:assert/strict';
import vm from 'node:vm';

const html=await readFile(new URL('../src/kit_wifi.h',import.meta.url),'utf8');
const script=html.match(/<script>([\s\S]*?)<\/script>/)[1];
class Element {
  children=[];textContent='';value='';disabled=false;hidden=false;
  replaceChildren(){this.children=[]}
  appendChild(child){this.children.push(child)}
}
async function run(options={}){
  const ids=['kit_wifi','kit_wifi_source','kit_wifi_manual','kit_wifi_ssid','kit_wifi_password',
    'kit_wifi_submit','kit_wifi_status','kit_wifi_log'];
  const elements=Object.fromEntries(ids.map(id=>[id,new Element()]));
  elements.kit_wifi_source.value=options.manual ? 'manual' : 'saved';
  elements.kit_wifi_ssid.value='Home & Network';
  elements.kit_wifi_password.value='p+ass&word';
  const requests=[];
  const nodes=[{id:'HOST',wifi_host:true,online:true},{id:'MEMBER',online:true},
    {id:'SCALE_ONLY',preference:2,update_available:false,online:true}];
  if(options.offline)nodes.push({id:'OFFLINE',online:false});
  const fetch=async (url,request)=>{
    if(url==='/data')return {ok:true,json:async()=>({role:options.wrongRole ? 'member' : 'gateway',nodes})};
    assert.equal(url,'/kit-wifi');assert.equal(request.method,'POST');
    requests.push(request);
    const id=request.body.get('id'),failed=options.fail===id;
    return {ok:!failed,json:async()=>failed ? {error:'Not saved'} : {ok:true}};
  };
  vm.runInNewContext(script,{
    document:{getElementById:id=>elements[id],createElement:()=>new Element()},
    URLSearchParams,fetch,AbortSignal:{timeout:()=>undefined}
  });
  assert.equal(elements.kit_wifi_manual.disabled,!options.manual);
  assert.equal(elements.kit_wifi_manual.hidden,!options.manual);
  const pending=elements.kit_wifi.onsubmit({preventDefault(){}});
  await elements.kit_wifi.onsubmit({preventDefault(){}}); // Double-click must not start another run.
  await pending;
  assert.equal(elements.kit_wifi_submit.disabled,false);
  assert.equal(elements.kit_wifi_password.value,'');
  return {elements,requests};
}
const saved=await run();
assert.deepEqual(saved.requests.map(r=>r.body.get('id')),['MEMBER','SCALE_ONLY','HOST']);
assert.match(saved.elements.kit_wifi_status.textContent,/saved on 3 of 3/);
for(const request of saved.requests){
  assert.equal(request.body.get('source'),'saved');
  assert.equal(request.body.has('password'),false); // Saved password stays on the gateway.
  assert.equal(request.body.has('ssid'),false);
}
const manual=await run({manual:true});
for(const request of manual.requests){
  assert.equal(request.body.get('source'),'manual');
  assert.equal(request.body.get('ssid'),'Home & Network');
  assert.equal(request.body.get('password'),'p+ass&word');
}
const failed=await run({fail:'MEMBER'});
assert.equal(failed.requests.length,3); // Continue to the other member and gateway.
assert.match(failed.elements.kit_wifi_status.textContent,/saved on 2 of 3/);
assert.match(failed.elements.kit_wifi_log.children[0].textContent,/MEMBER: Not saved/);
const offline=await run({offline:true});
assert.equal(offline.requests.length,3);
assert.match(offline.elements.kit_wifi_status.textContent,/saved on 3 of 4/);
assert.ok(offline.elements.kit_wifi_log.children.some(row=>/OFFLINE: offline/.test(row.textContent)));
const wrongRole=await run({wrongRole:true});
assert.equal(wrongRole.requests.length,0);
assert.match(wrongRole.elements.kit_wifi_status.textContent,/current gateway/);
console.log('PASS: saved/manual kit Wi-Fi sharing, Scale-only inclusion, gateway last, partial failures and no password disclosure');
