// Compile the production credential storage and HTTP handlers with controlled peers/NVS.
#include "scale_network.h"
#include "wifi_credentials.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <map>
#include <sstream>
#include <string>
#include <vector>
using namespace scales;

struct String {
  std::string value;
  String() = default;
  String(const char *s):value(s) {}
  String(std::string s):value(s) {}
  size_t length() const { return value.size(); }
  const char *c_str() const { return value.c_str(); }
  String &operator+=(char c) { value+=c;return *this; }
  friend String operator+(const String &a,const String &b) { return a.value+b.value; }
  friend bool operator==(const String &a,const String &b) { return a.value==b.value; }
  friend bool operator!=(const String &a,const String &b) { return !(a==b); }
};
struct {
  std::map<std::string,std::vector<uint8_t>> blobs;
  std::map<std::string,String> strings;
  bool fail=false;unsigned writes=0;
  bool isKey(const char *key) { return blobs.count(key) || strings.count(key); }
  size_t putBytes(const char *key,const void *data,size_t length) {
    if(fail)return 0;
    ++writes;
    const auto *bytes=static_cast<const uint8_t *>(data);
    blobs[key]=std::vector<uint8_t>(bytes,bytes+length);return length;
  }
  size_t getBytesLength(const char *key) { return blobs[key].size(); }
  size_t getBytes(const char *key,void *data,size_t length) {
    auto &bytes=blobs[key];if(bytes.size()>length)return 0;
    memcpy(data,bytes.data(),bytes.size());return bytes.size();
  }
  String getString(const char *key,const char *fallback) { return strings.count(key)?strings[key]:String(fallback); }
} prefs;
String homeSsid,homePassword;
struct wifi_config_t { struct { uint8_t ssid[32]{},password[64]{}; } sta; } sdkCredentials;
constexpr int WIFI_IF_STA=0,ESP_OK=0,WL_CONNECTED=3;
bool sdkAvailable=false;
int esp_wifi_get_config(int,wifi_config_t *out) { *out=sdkCredentials;return sdkAvailable?ESP_OK:-1; }
struct IPAddress {
  uint32_t value;
  IPAddress(uint32_t v):value(v) {}
  IPAddress(int a,int b,int c,int d):value(a|(b<<8)|(c<<16)|(uint32_t(d)<<24)) {}
  explicit operator uint32_t() const { return value; }
  bool operator==(IPAddress other) const { return value==other.value; }
  String toString() const { return "192.168.4.2"; }
};
struct {
  int state=WL_CONNECTED;
  int status() { return state; }
  IPAddress gatewayIP() { return IPAddress(192,168,4,1); }
  IPAddress softAPIP() { return gatewayIP(); }
  IPAddress softAPSubnetMask() { return IPAddress(255,255,255,0); }
} WiFi;
IPAddress remote(192,168,4,1);
struct WiFiClient { IPAddress remoteIP() { return remote; } } peer;
struct {
  std::map<std::string,String> args;
  int status=0;String body;
  String arg(const char *key) { return args[key]; }
  WiFiClient &client() { return peer; }
  void sendHeader(const char*,const char*) {}
  void send(int code,const char*,const String &text) { status=code;body=text; }
} httpServer;
decltype(httpServer) &server() { return httpServer; }
struct { Role current=Role::Gateway;Role role() { return current; } } policy;
auto election=&policy;
struct { bool active=false; } operation;
uint32_t restartAt=0;
uint64_t selfId=0x111111111111;
String idText(uint64_t id) { std::ostringstream s;s<<std::hex<<id;return s.str(); }
struct Node { uint64_t id=0;Message reading{}; } nodes[2];
int postStatus=200,posts=0,heartbeats=0;
String postUrl,postBody,postResponse="{\"ok\":true,\"id\":\"222222222222\"}";
struct HTTPClient {
  void setConnectTimeout(int) {}
  void setTimeout(int) {}
  bool begin(WiFiClient&,const String &url) { postUrl=url;return true; }
  void addHeader(const char*,const char*) {}
  int POST(const String &body) { ++posts;postBody=body;return postStatus; }
  String getString() { return postResponse; }
  void end() {}
};
void relayHeartbeat() { ++heartbeats; }
#include "wifi_credentials_functions.h"

void storageTests() {
  HomeCredentials c;
  assert(makeHomeCredentials("My & Network", "a+b%&=\"'",c));
  assert(storeHomeCredentials(c));
  homeSsid="";homePassword="";loadHomeCredentials();
  assert(homeSsid=="My & Network" && homePassword=="a+b%&=\"'");
  prefs.fail=true;
  assert(makeHomeCredentials("New", "password",c));
  assert(!storeHomeCredentials(c) && homeSsid=="My & Network");
  prefs.fail=false;
  assert(makeHomeCredentials("Open", "",c));assert(storeHomeCredentials(c));
  homeSsid="";homePassword="old";loadHomeCredentials();
  assert(homeSsid=="Open" && homePassword=="");
  assert(!makeHomeCredentials("", "password",c));
  assert(!makeHomeCredentials(std::string(33,'s'), "password",c));
  assert(!makeHomeCredentials("Network", "short",c));
  assert(makeHomeCredentials(std::string(32,'s'), std::string(63,'p'),c));
  assert(makeHomeCredentials("Network", std::string(64,'A'),c));
  assert(!makeHomeCredentials("Network", std::string(64,'z'),c));
  assert(!makeHomeCredentials(std::string("a\0b",3), "password",c));
  assert(!makeHomeCredentials("Network", std::string("password\0bad",12),c));
  prefs.blobs.clear();prefs.strings["ssid"]="Legacy";prefs.strings["password"]="legacy-pass";
  loadHomeCredentials();assert(homeSsid=="Legacy" && homePassword=="legacy-pass");
  prefs.strings.clear();sdkAvailable=true;
  memcpy(sdkCredentials.sta.ssid,"SDK",3);memcpy(sdkCredentials.sta.password,"sdk-pass",8);
  // A malformed blob must not contaminate zero termination when falling back to the SDK.
  prefs.blobs["home_wifi"]=std::vector<uint8_t>(sizeof(HomeCredentials),'x');
  loadHomeCredentials();assert(homeSsid=="SDK" && homePassword=="sdk-pass");
  assert(prefs.blobs["home_wifi"].size()==sizeof(HomeCredentials));
  assert(encodeWifiField("a b+%&=\"'")=="a%20b%2B%25%26%3D%22%27");
  assert(encodeWifiField("\xC3\xA9")=="%C3%A9");
}
void memberTests() {
  policy.current=Role::Member;
  httpServer.args={{"id",idText(selfId)},{"ssid","Shared"},{"password","shared-pass"}};
  unsigned before=prefs.writes;
  handleWifiCredentials();assert(httpServer.status==200 && prefs.writes==before+1);
  assert(homeSsid=="Shared" && homePassword=="shared-pass");
  assert(httpServer.body=="{\"ok\":true,\"id\":\"111111111111\"}");
  before=prefs.writes;
  httpServer.args["id"]="WRONG";handleWifiCredentials();assert(httpServer.status==400 && prefs.writes==before);
  httpServer.args["id"]=idText(selfId);
  remote=IPAddress(192,168,4,9);handleWifiCredentials();assert(httpServer.status==403 && prefs.writes==before);
  remote=IPAddress(192,168,4,1);WiFi.state=0;handleWifiCredentials();assert(httpServer.status==403);
  WiFi.state=WL_CONNECTED;prefs.fail=true;handleWifiCredentials();assert(httpServer.status==500);
  prefs.fail=false;
}
void gatewayTests() {
  policy.current=Role::Gateway;
  nodes[0].id=selfId;nodes[1].id=0x222222222222;
  nodes[1].reading.ip=static_cast<uint32_t>(IPAddress(192,168,4,2));
  nodes[1].reading.preference=Preference::ScaleOnly; // Credential sharing is independent of OTA role policy.
  HomeCredentials c;assert(makeHomeCredentials("Home & WiFi", "p+ass&word",c));assert(storeHomeCredentials(c));
  httpServer.args={{"id",idText(nodes[1].id)},{"source","saved"}};
  handleKitWifi();assert(httpServer.status==200 && posts==1 && heartbeats==2);
  assert(postUrl=="http://192.168.4.2/wifi-credentials");
  assert(postBody=="id=222222222222&ssid=Home%20%26%20WiFi&password=p%2Bass%26word");
  assert(homeSsid=="Home & WiFi"); // Relay does not change gateway settings.
  postResponse="{\"ok\":true,\"id\":\"WRONG\"}";handleKitWifi();assert(httpServer.status==502);
  postStatus=404;handleKitWifi();assert(httpServer.body.value.find("1.2.11")!=std::string::npos);
  postStatus=200;postResponse="{\"ok\":true,\"id\":\"222222222222\"}";
  int before=posts;
  nodes[1].reading.ip=0;handleKitWifi();assert(httpServer.status==409 && posts==before);
  nodes[1].reading.ip=static_cast<uint32_t>(IPAddress(192,168,10,22));
  handleKitWifi();assert(httpServer.status==409 && posts==before);
  httpServer.args={{"id",idText(selfId)},{"source","manual"},{"ssid","Manual"},{"password",""}};
  handleKitWifi();assert(httpServer.status==200 && homeSsid=="Manual" && homePassword=="");
  operation.active=true;handleKitWifi();assert(httpServer.status==409);operation.active=false;
  httpServer.args["source"]="bad";handleKitWifi();assert(httpServer.status==400);
  httpServer.args["source"]="saved";homeSsid="";handleKitWifi();assert(httpServer.status==400);
}
int main() {
  storageTests();memberTests();gatewayTests();
  puts("PASS: atomic Wi-Fi credential persistence, legacy migration, validation, peer identity, Scale-only sharing and failures");
}
