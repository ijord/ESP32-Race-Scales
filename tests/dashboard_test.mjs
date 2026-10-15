// Exercise production page scripts without a browser or connected scales.
import {readFile} from 'node:fs/promises';
import assert from 'node:assert/strict';
import vm from 'node:vm';

class Element {
  children=[]; textContent=''; innerText=''; className='';
  classList={add(){}};
  set innerHTML(value){this.children=[]}
  appendChild(child){this.children.push(child)}
  replaceChildren(){this.children=[]}
}
const html=await readFile(new URL('../src/dashboard.h',import.meta.url),'utf8');
assert.ok(html.indexOf('id="live_status"')<html.indexOf('id="kit_status"'));
assert.ok(html.indexOf('id="node_list"')<html.indexOf('id="page_links"'));
assert.match(html,/<th>Zero offset \(lb\)<\/th>/);
const elements=Object.fromEntries([...html.matchAll(/id="([^"]+)"/g)].map(m=>[m[1],new Element()]));
let render,status;
const context={...elements,document:{getElementById:id=>elements[id],createElement:()=>new Element()},
  connectLive:(path,onData,onStatus)=>{assert.equal(path,'/scales');render=onData;status=onStatus}};
vm.runInNewContext(html.match(/<script>([\s\S]*?)<\/script>/)[1],context);
const data={kit:'SCALE',firmware_version:'1.2.0',online_count:2,balance_available:false,
  lf:-12.5,lf_online:true,lf_locked:false,rf:0,rf_online:true,rf_locked:false,
  lr:0,lr_online:false,lr_locked:false,rr:0,rr_online:false,rr_locked:false,
  total:-12.5,front:-12.5,rear:0,left:-12.5,right:0,
  nodes:[{id:'LF',firmware_version:'1.2.0',wifi_host:true,calibration:-2978.173096,offset:-178485.2,
    corner:1,online:true,ready:true,calibrated:true,weight:-12.5},
    {id:'RF',corner:2,online:true,ready:true,calibrated:true,weight:0}]};
const digits=id=>elements[id].children.map(c=>c.innerText).join('').trim();
render(data);
assert.equal(digits('lf'),'-12.5');
assert.equal(digits('rf'),'0.0');
assert.equal(digits('lr'),'--');
assert.equal(elements.total.innerText,'-12.5');
assert.match(elements.kit_status.textContent,/Gateway v1\.2\.0/);
assert.equal(elements.node_list.children[0].children[0].textContent,'LF (WiFi)');
assert.equal(elements.node_list.children[1].children[0].textContent,'RF');
assert.equal(elements.node_list.children[0].children[1].textContent,'1.2.0');
assert.equal(elements.node_list.children[0].children[3].textContent,'-12.5 lb');
assert.equal(elements.node_list.children[0].children[4].textContent,'-2978.173096');
assert.equal(elements.node_list.children[0].children[5].textContent,'59.9');
assert.equal(elements.node_list.children[1].children[4].textContent,'—');
assert.equal(elements.node_list.children[1].children[1].textContent,'unknown');
assert.equal(elements.lf_batt.textContent,'Battery: --'); // Older firmware has no battery telemetry.
data.nodes[0].battery_voltage=4.18;
data.nodes[1].battery_voltage=3.72;
render(data);
assert.equal(elements.lf_batt.textContent,'Battery: 4.18 V');
assert.equal(elements.rf_batt.textContent,'Battery: 3.72 V');
assert.equal(elements.lr_batt.textContent,'Battery: --');
assert.equal(elements.rr_batt.textContent,'Battery: --');
data.nodes[0].ready=false;data.nodes[0].calibrated=false;render(data);
assert.equal(elements.lf_batt.textContent,'Battery: 4.18 V'); // Battery does not depend on the HX711.
data.nodes[0].online=false;render(data);
assert.equal(elements.lf_batt.textContent,'Battery: --');
data.nodes[0].online=true;data.nodes[0].ready=true;data.nodes[0].calibrated=true;
data.nodes[1].corner=1;render(data);
assert.equal(elements.lf_batt.textContent,'Battery: --'); // Do not pick an arbitrary duplicate corner.
data.nodes[1].corner=2;
data.nodes.push({corner:3,online:true,battery_voltage:3.95},{corner:4,online:true,battery_voltage:0});
render(data);
assert.equal(elements.lr_batt.textContent,'Battery: 3.95 V');
assert.equal(elements.rr_batt.textContent,'Battery: 0.00 V');
data.nodes.splice(2);
context.toggleUnits();
assert.equal(digits('lf'),'-5.7');
assert.equal(elements.node_list.children[0].children[4].textContent,'-2978.173096'); // Counts/lb stays explicit.
assert.equal(elements.node_list.children[0].children[5].textContent,'59.9'); // Zero offset remains in pounds.
data.nodes[0].calibration=0;render(data);
assert.equal(elements.node_list.children[0].children[5].textContent,'—');
data.nodes[0].calibration=2978.173096;render(data);
assert.equal(elements.node_list.children[0].children[5].textContent,'-59.9');
data.nodes[0].calibration=-2978.173096;
status('Disconnected');
assert.equal(digits('lf'),'--');
assert.equal(elements.total.innerText,'--');
assert.equal(elements.lf_batt.textContent,'Battery: --');
assert.equal(elements.rf_batt.textContent,'Battery: --');

const uploadHtml=await readFile(new URL('../src/member_update.h',import.meta.url),'utf8');
const uploadElements=Object.fromEntries(['upload','file','submit','status','progress'].map(id=>[id,new Element()]));
const file={name:'firmware.bin',size:967520};
uploadElements.file.files=[file];
let request;
class Request {
  upload={};
  constructor(){request=this}
  open(method,path){this.method=method;this.path=path}
  setRequestHeader(name,value){this.header=[name,value]}
  send(body){this.body=body}
}
vm.runInNewContext(uploadHtml.match(/<script>([\s\S]*?)<\/script>/)[1],{
  document:{getElementById:id=>uploadElements[id]},
  location:{pathname:'/member-update/A0F262F4BA28'},XMLHttpRequest:Request});
const submit=()=>uploadElements.upload.onsubmit({preventDefault(){}});
submit();
assert.equal(request.path,'/member-update/A0F262F4BA28');
assert.equal(request.method,'POST');
assert.deepEqual(request.header,['Content-Type','application/octet-stream']);
assert.equal(request.body,file);
request.upload.onprogress({lengthComputable:true,loaded:50,total:100});
assert.equal(uploadElements.progress.value,50);
request.status=502;request.responseText='{"ok":false,"message":"Wrong scale identity"}';request.onload();
assert.equal(uploadElements.status.textContent,'Wrong scale identity');
assert.equal(uploadElements.submit.disabled,false);
submit();request.status=200;request.responseText='{"ok":true}';request.onload();
assert.match(uploadElements.status.textContent,/selected scale is rebooting/);
assert.equal(uploadElements.submit.disabled,true);
submit();request.ontimeout();
assert.equal(uploadElements.submit.disabled,false);
assert.match(uploadElements.status.textContent,/Check the selected scale/);
console.log('PASS: signed weights, page ordering, pound-based zero offset, versions, offline values, member upload form');
