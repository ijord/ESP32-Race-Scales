// Compile production relay handlers against bounded, controllable transports.
#include "scale_network.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <map>
using namespace scales;

struct String {
  std::string value;
  String() = default;
  String(const char *s): value(s) {}
  String(std::string s): value(s) {}
  String(uint64_t number, int base=10) {
    std::ostringstream stream;
    if (base==16) stream<<std::hex;
    stream<<number; value=stream.str();
  }
  const char *c_str() const { return value.c_str(); }
  size_t length() const { return value.size(); }
  int indexOf(const String &s) const { auto p=value.find(s.value);return p==std::string::npos?-1:static_cast<int>(p); }
  bool startsWith(const char *s) const { return value.find(s)==0; }
  bool equalsIgnoreCase(const String &s) const { return value==s.value; }
  void concat(const char *data, size_t size) { value.append(data,size); }
  void replace(const char *from, const String &to) { auto p=value.find(from);if(p!=std::string::npos)value.replace(p,strlen(from),to.value); }
  friend String operator+(const String &a,const String &b) { return a.value+b.value; }
  friend bool operator!=(const String &a,const String &b) { return a.value!=b.value; }
};
struct IPAddress {
  uint32_t value;
  IPAddress(uint32_t v):value(v) {}
  IPAddress(int a,int b,int c,int d):value(a|(b<<8)|(c<<16)|(uint32_t(d)<<24)) {}
  explicit operator uint32_t() const { return value; }
  bool operator==(IPAddress other) const { return value==other.value; }
  String toString() const { return "192.168.4.2"; }
};
struct {
  IPAddress softAPIP() { return IPAddress(192,168,4,1); }
  IPAddress softAPSubnetMask() { return IPAddress(255,255,255,0); }
  void scanDelete() {}
} WiFi;
bool connectOk=true,writeOk=true,identityOk=true,nodeOnline=true;
std::string outgoing,response="HTTP/1.1 200 OK\r\n\r\nUpdate successful.";
struct WiFiClient {
  bool open=false;
  size_t position=0;
  void stop() { open=false; }
  void setTimeout(int) {}
  bool connect(IPAddress,int,int) { open=connectOk;position=0;return open; }
  size_t write(const uint8_t *data,size_t size) {
    if(!open || !writeOk)return 0;
    size=std::min(size,size_t(17)); // Exercise partial TCP writes.
    outgoing.append(reinterpret_cast<const char*>(data),size);return size;
  }
  bool connected() { return open && position<response.size(); }
  size_t available() { return open?response.size()-position:0; }
  int read(uint8_t *data,size_t size) {
    size=std::min(size,available());memcpy(data,response.data()+position,size);position+=size;return static_cast<int>(size);
  }
};
struct HTTPClient {
  void setConnectTimeout(int) {}
  void setTimeout(int) {}
  void begin(WiFiClient&,const String&) {}
  int GET() { return 200; }
  String getString() { return identityOk?"{\"id\":\"A0F262F4BA28\"}":"{\"id\":\"WRONG\"}"; }
  void end() {}
};
enum HTTPRawStatus { RAW_START,RAW_WRITE,RAW_END,RAW_ABORTED };
struct HTTPRaw { HTTPRawStatus status;size_t currentSize=0;uint8_t buf[1436]; };
struct Server {
  HTTPRaw upload{};
  std::map<std::string,String> headers;
  int status=0;String body;bool allowRaw=true;
  HTTPRaw &raw() { assert(allowRaw);return upload; }
  String pathArg(int) { return "A0F262F4BA28"; }
  String header(const char *key) { return headers[key]; }
  void sendHeader(const char*,const char*) {}
  void send(int code,const char*,const String &data) { status=code;body=data; }
} httpServer;
Server &server() { return httpServer; }
struct Node { uint64_t id;Message reading{}; };
Node nodes[1];
bool online(const Node&) { return nodeOnline; }
uint64_t selfId=0xE072A1E9831C;
String idText(uint64_t id) { return id==selfId?"E072A1E9831C":"A0F262F4BA28"; }
const char *CornerNames[]={"Unassigned","LF","RF","LR","RR"};
struct { Role current=Role::Gateway;Role role() { return current; } } policy;
auto election=&policy;
struct { bool active=false; } operation;
uint32_t restartAt=0,lastNetworkSend=0,lastScan=0,now=250;
bool scanRunning=false;
uint32_t millis() { return now; }
void yield() {}
void delay(int duration) { now+=duration; }
int esp_random() { return 12345; }
const int HEX=16;
struct esp_partition_t { size_t size=1310720; } slot;
const esp_partition_t *esp_ota_get_next_update_partition(void*) { return &slot; }
void esp_wifi_scan_stop() {}
const uint8_t Broadcast[6]={255,255,255,255,255,255};
uint64_t nodeId(const uint8_t*) { return 1; }
Message message(Kind kind) { Message m{};m.kind=kind;return m; }
int heartbeats=0;
void enqueue(uint64_t,const Message&) { ++heartbeats; }
void processTransmits() {}
#define PROGMEM
#define FPSTR(value) value
#include "../src/member_update.h"
#include "member_update_functions.h"

void start(size_t length=64) {
  outgoing.clear();response="HTTP/1.1 200 OK\r\n\r\nUpdate successful.";
  httpServer.headers["Content-Length"]=String(length);
  httpServer.headers["Content-Type"]="application/octet-stream";
  httpServer.upload.status=RAW_START;handleMemberUpload();
}
void chunk(size_t length=64,uint8_t magic=0xe9) {
  httpServer.upload.status=RAW_WRITE;httpServer.upload.currentSize=length;
  memset(httpServer.upload.buf,0x42,length);httpServer.upload.buf[0]=magic;
  handleMemberUpload();
}
void finish() {
  httpServer.upload.status=RAW_END;handleMemberUpload();handleMemberUploadDone();
}
int main() {
  nodes[0].id=0xA0F262F4BA28;nodes[0].reading.ip=static_cast<uint32_t>(IPAddress(192,168,4,2));
  nodes[0].reading.corner=Corner::LF;
  nodes[0].reading.preference=Preference::Auto;
  start();assert(memberUpload.error.length()==0);chunk();finish();
  assert(httpServer.status==200 && memberUpload.complete && heartbeats>0);
  auto headersEnd=outgoing.find("\r\n\r\n");assert(headersEnd!=std::string::npos);
  auto lengthStart=outgoing.find("Content-Length: ")+16;
  assert(std::stoul(outgoing.substr(lengthStart))==outgoing.size()-headersEnd-4);
  assert(outgoing.find("POST /u HTTP/1.1\r\n")==0);
  assert(outgoing.find("name=\"update\"; filename=\"firmware.bin\"")!=std::string::npos);
  assert(outgoing.find(std::string(1,char(0xe9))+std::string(63,'B'))!=std::string::npos);
  nodes[0].reading.ip=0;
  start();chunk();finish();assert(httpServer.status==502 && outgoing.empty());
  assert(httpServer.body.indexOf("ESP-NOW readings alone")>=0);
  nodes[0].reading.ip=static_cast<uint32_t>(IPAddress(192,168,4,2));
  identityOk=false;start();assert(outgoing.empty());chunk();finish();assert(httpServer.status==502);
  // Simulate cached readings timing out while the synchronous HTTP parser runs.
  // The member's HTTP identity still proves it is reachable, so the relay must work.
  identityOk=true;nodeOnline=false;start();chunk();finish();
  assert(httpServer.status==200 && memberUpload.complete);
  nodeOnline=true;
  nodes[0].reading.preference=Preference::ScaleOnly;
  handleMemberUpdatePage();assert(httpServer.status==403);
  start();assert(outgoing.empty() && memberUpload.error.indexOf("Scale only")>=0);
  nodes[0].reading.preference=Preference::Auto;
  operation.active=true;start();assert(outgoing.empty() && memberUpload.error.indexOf("scale command")>=0);
  operation.active=false;
  restartAt=1;start();assert(outgoing.empty() && memberUpload.error.indexOf("restarting")>=0);
  restartAt=0;
  start(1310721);assert(outgoing.empty());
  start();chunk(64,0);finish();assert(httpServer.status==502 && !memberUpload.complete);
  start();chunk(65);finish();assert(httpServer.status==502);
  start();chunk(32);finish();assert(httpServer.status==502);
  start();chunk();response="HTTP/1.1 200 OK\r\n\r\nUpdate failed!";finish();assert(httpServer.status==502);
  writeOk=false;start();assert(memberUpload.error.length());writeOk=true;
  start();httpServer.upload.status=RAW_ABORTED;handleMemberUpload();assert(!memberUpload.client.open);
  httpServer.headers["Content-Type"]="multipart/form-data";httpServer.allowRaw=false;
  handleMemberUpload();handleMemberUploadDone();assert(httpServer.status==502);
  httpServer.allowRaw=true;
  policy.current=Role::Member;start();assert(outgoing.empty());
  puts("PASS: member OTA relay, stale readings with verified identity, role policy, busy errors, partial writes and failures");
}
