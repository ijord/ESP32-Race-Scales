/*
traviscea DIY Race Scales
Copyright (c) 2026 Travis Way
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <uri/UriBraces.h>
#include <ESPmDNS.h>
#include <WebSocketsServer.h>
#include <Preferences.h>
#include <HX711.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_ota_ops.h>
#include <atomic>
#include <memory>
#include <freertos/timers.h>
#include "scale_network.h"
#include "firmware_version.h"
#include "dashboard.h"
#include "diagnostics.h"
#include "live_updates.h"
#include "member_update.h"
#include "kit_update.h"

using namespace scales;

constexpr int HX_DT = 12, HX_SCK = 13, BOOT_PIN = 0;
constexpr int BATTERY_PIN = 3;
constexpr int STATUS_RGB_PIN = 48;
constexpr uint32_t NodeTimeoutMs = 3000;
const char *AP_PASSWORD = "12345678";
const uint8_t Broadcast[6] = {255, 255, 255, 255, 255, 255};
const char *CornerNames[] = {"Unassigned", "LF", "RF", "LR", "RR"};
const char *PreferenceNames[] = {"Auto", "Prefer gateway", "Scale only"};

Preferences prefs;
HX711 sensor;
WiFiManager wifiManager;
WebSocketsServer liveSocket(81);
uint8_t liveStreams[WEBSOCKETS_SERVER_CLIENT_MAX]{}; // 1: dashboard, 2: diagnostics.
std::unique_ptr<Election> election;
CommandHistory commandHistory;
char kit[9] = "SCALE";
Corner corner = Corner::Unassigned;
Preference preference = Preference::Auto;
uint64_t selfId;
uint32_t bootId, sequence = 0, restartAt = 0;
String homeSsid, homePassword, hotspot;
float calibration = 0, offset = 0, rawReading = 0, filteredWeight = 0, stableWeight = 0;
bool haveOffset = false, haveReading = false, locked = false;
uint32_t batteryMillivolts = 0;
bool resetSamples = false;
uint32_t lastSample = 0, lastNetworkSend = 0, lastHomeRetry = 0, lastScan = 0;
uint32_t scanStartedAt = 0;
bool scanRunning = false, wasHomeConnected = false;
std::atomic<bool> ledGateway{false};
uint32_t receivedFrames = 0, rejectedFrames = 0;
esp_err_t lastSendError = ESP_OK;
bool mdnsActive = false;
uint32_t lastMdnsAttempt = 0;
bool dualDiagnostics = false;
uint32_t diagnosticsRenewed = 0, channelChangedAt = 0, conversionCount = 0;
float conversionHz = 0;
uint8_t sampleGain = 128, nextGain = 128;
struct RawChannel {
  long value = 0;
  uint32_t sampledAt = 0, count = 0;
};
RawChannel rawChannels[2]; // A/128 and B/32, before averaging, offset, or filtering.

struct Node {
  uint64_t id = 0;
  Message reading{};
  uint32_t lastSeen = 0;
};
Node nodes[4]; // Slot zero always belongs to this physical scale.

struct Frame {
  uint8_t mac[6];
  Message message;
};
QueueHandle_t receiveQueue, transmitQueue;
std::atomic<bool> transmitBusy{false};
std::atomic<int> lastDeliveryStatus{-1};

struct PendingCommand {
  uint64_t target = 0;
  Message message{};
  uint32_t sentAt = 0;
  bool done = false;
  Result result = Result::Timeout;
};
struct Operation {
  uint32_t id = 0, started = 0;
  bool active = false;
  size_t count = 0;
  PendingCommand commands[4];
} operation;

uint64_t nodeId(const uint8_t *mac) {
  uint64_t id = 0;
  for (int i = 0; i < 6; ++i) id = (id << 8) | mac[i];
  return id;
}

String idText(uint64_t id) {
  char text[13];
  snprintf(text, sizeof(text), "%012llX", static_cast<unsigned long long>(id));
  return text;
}

uint32_t nextSequence() {
  if (++sequence == 0) ++sequence;
  return sequence;
}

WebServer &server() { return *wifiManager.server; }

bool online(const Node &node) {
  return node.id && millis() - node.lastSeen < NodeTimeoutMs;
}

bool ready(const Node &node) { return online(node) && (node.reading.flags & Ready); }

Node *findNode(uint64_t id, bool create = false) {
  for (Node &node : nodes) if (node.id == id) return &node;
  if (!create) return nullptr;
  for (size_t i = 1; i < 4; ++i) {
    if (!nodes[i].id || millis() - nodes[i].lastSeen > 15000) {
      nodes[i] = Node{};
      nodes[i].id = id;
      return &nodes[i];
    }
  }
  return nullptr;
}

Message message(Kind kind) {
  Message m{};
  m.magic = Magic; m.version = Version; m.kind = kind;
  m.boot = bootId; m.sequence = nextSequence();
  memcpy(m.kit, kit, sizeof(kit));
  m.preference = preference; m.corner = corner;
  m.channel = WiFi.channel();
  if (!m.channel) m.channel = 1;
  if (kind == Kind::Beacon || kind == Kind::Reading)
    memcpy(m.firmwareVersion, SCALE_FIRMWARE_VERSION, sizeof(SCALE_FIRMWARE_VERSION));
  return m;
}

bool enqueue(uint64_t target, const Message &m) {
  Frame frame{};
  for (int i = 5; i >= 0; --i) { frame.mac[i] = target & 255; target >>= 8; }
  frame.message = m;
  return xQueueSend(transmitQueue, &frame, 0) == pdTRUE;
}

void onReceive(const uint8_t *mac, const uint8_t *bytes, int length) {
  if (length != sizeof(Message) || (mac[0] & 1)) return;
  Frame frame;
  memcpy(frame.mac, mac, 6);
  memcpy(&frame.message, bytes, sizeof(frame.message));
  xQueueSend(receiveQueue, &frame, 0);
}

void onSent(const uint8_t *, esp_now_send_status_t status) {
  lastDeliveryStatus.store(status);
  transmitBusy.store(false);
}

void processTransmits() {
  if (transmitBusy.load()) return;
  Frame frame;
  if (xQueueReceive(transmitQueue, &frame, 0) != pdTRUE) return;
  if (!esp_now_is_peer_exist(frame.mac)) {
    esp_now_peer_info_t peer{};
    memcpy(peer.peer_addr, frame.mac, 6);
    peer.ifidx = WIFI_IF_STA;
    peer.channel = 0;
    lastSendError = esp_now_add_peer(&peer);
    if (lastSendError != ESP_OK) return;
  }
  transmitBusy.store(true);
  lastSendError = esp_now_send(frame.mac, reinterpret_cast<uint8_t *>(&frame.message),
                              sizeof(frame.message));
  if (lastSendError != ESP_OK) transmitBusy.store(false);
}

void updateBatteryVoltage() {
  static uint32_t sampledAt = 0;
  static bool sampled = false;
  uint32_t now = millis();
  if (sampled && now - sampledAt < 1000) return;
  uint32_t sum = 0;
  for (unsigned i = 0; i < 8; ++i) sum += analogReadMilliVolts(BATTERY_PIN);
  // Undo the 2:1 divider. Above 4.5 V, VBUS is USB-powered and bypasses the diode.
  batteryMillivolts = (sum * 2 + 4) / 8;
  if (batteryMillivolts <= 4500) batteryMillivolts += 970;
  sampledAt = now;
  sampled = true;
}

void updateLocalReading() {
  static int64_t sum = 0;
  static uint8_t count = 0;
  static uint32_t rateStarted = 0, rateConversions = 0;
  uint32_t now = millis();
  if (resetSamples) {
    sum = 0; count = 0; haveReading = false; locked = false;
    resetSamples = false;
  }
  if (dualDiagnostics && now - diagnosticsRenewed >= 10000) dualDiagnostics = false;
  if (!dualDiagnostics) nextGain = 128;
  if (dualDiagnostics || sampleGain != 128 || now - channelChangedAt < 400) {
    sum = 0; count = 0; haveReading = false; locked = false;
  }
  if (sensor.is_ready()) {
    // set_gain selects the NEXT conversion, not the value being shifted out.
    sensor.set_gain(nextGain);
    long value = sensor.read();
    lastSample = now;
    ++conversionCount;
    if (sampleGain != nextGain) {
      sampleGain = nextGain; channelChangedAt = now;
    } else if (now - channelChangedAt >= 400) {
      RawChannel &channel = rawChannels[sampleGain == 128 ? 0 : 1];
      channel.value = value; channel.sampledAt = now; ++channel.count;
      if (dualDiagnostics) nextGain = sampleGain == 128 ? 32 : 128;
      else { sum += value; ++count; }
    }
    if (count == 10) {
      rawReading = sum / 10.0f;
      sum = 0; count = 0; haveReading = true;
      if (!haveOffset) {
        offset = rawReading; haveOffset = true;
        prefs.putFloat("offset", offset);
      }
      if (fabsf(calibration) > 0.0001f) {
        float weight = (rawReading - offset) / calibration;
        filteredWeight += 0.8f * (weight - filteredWeight);
        float diff = fabsf(filteredWeight - stableWeight);
        if (diff < 0.5f) locked = true;
        else if (diff > 1.0f) { locked = false; stableWeight = filteredWeight; }
      }
    }
  } else if (now - lastSample > 2000) {
    // Discard a partial average across an unplug/replug interval.
    sum = 0; count = 0; haveReading = false; locked = false;
    sampleGain = nextGain = 128; channelChangedAt = now;
  }
  if (now - rateStarted >= 1000) {
    conversionHz = (conversionCount - rateConversions) * 1000.0f / (now - rateStarted);
    rateStarted = now; rateConversions = conversionCount;
  }
  Node &local = nodes[0];
  local.id = selfId; local.lastSeen = now;
  local.reading = message(Kind::Reading);
  if (haveReading && now - lastSample < 2000) local.reading.flags |= Ready;
  if (fabsf(calibration) > 0.0001f) local.reading.flags |= Calibrated;
  if (locked) local.reading.flags |= Locked;
  local.reading.raw = rawReading;
  local.reading.weight = roundf(filteredWeight * 2) / 2;
  local.reading.ip = WiFi.status() == WL_CONNECTED ? static_cast<uint32_t>(WiFi.localIP()) : 0;
  local.reading.readingMetadataVersion = 4;
  local.reading.batteryMillivolts = batteryMillivolts;
  local.reading.calibration = calibration;
  local.reading.zeroOffset = offset;
}

Result applyCommand(const Message &m) {
  if (m.command == Command::Configure) {
    prefs.putString("kit", m.configKit);
    prefs.putUChar("corner", static_cast<uint8_t>(m.configCorner));
    prefs.putUChar("mode", static_cast<uint8_t>(m.configPreference));
    restartAt = millis() + 1500; // Allow the HTTP response/command ACK to leave first.
    return Result::Ok;
  }
  if (!ready(nodes[0])) return Result::Offline;
  if (m.command == Command::Tare) {
    offset = rawReading; haveOffset = true;
    prefs.putFloat("offset", offset);
  } else if (m.command == Command::Calibrate) {
    float factor = (rawReading - offset) / m.knownWeight;
    if (!std::isfinite(factor) || fabsf(factor) < 0.0001f) return Result::Invalid;
    calibration = factor;
    prefs.putFloat("cal", calibration);
  } else return Result::Invalid;
  filteredWeight = 0; stableWeight = 0; locked = false;
  return Result::Ok;
}

String gatewaySsid(uint64_t id, Preference mode);

void processReceives() {
  Frame frame;
  while (xQueueReceive(receiveQueue, &frame, 0) == pdTRUE) {
    ++receivedFrames;
    Message m;
    if (!decode(reinterpret_cast<uint8_t *>(&frame.message), sizeof(Message), m) ||
        strcmp(m.kit, kit)) { ++rejectedFrames; continue; }
    uint64_t sender = nodeId(frame.mac);
    if (!sender || sender == selfId) continue;
    if (m.kind == Kind::Beacon) {
      if (sender == election->gateway()) election->heartbeat(sender, millis());
      else if (election->role() != Role::Member) {
        // A valid beacon also discovers same-channel gateways without a scan.
        Candidate candidate{sender, m.preference};
        election->scanned(&candidate, 1, millis());
        if (election->role() == Role::Member)
          hotspot = gatewaySsid(sender, m.preference);
      }
    } else if (m.kind == Kind::Reading && election->role() == Role::Gateway) {
      Node *node = findNode(sender, true);
      if (!node) continue;
      if (node->reading.boot == m.boot &&
          !newer(m.sequence, node->reading.sequence)) continue;
      node->reading = m; node->lastSeen = millis();
    } else if (m.kind == Kind::Command && election->role() == Role::Member &&
               sender == election->gateway()) {
      Result result;
      if (commandHistory.duplicate(sender, m.boot, m.sequence)) result = commandHistory.result();
      else if (commandHistory.stale(sender, m.boot, m.sequence)) result = Result::Stale;
      else {
        result = applyCommand(m);
        commandHistory.remember(sender, m.boot, m.sequence, result);
      }
      Message ack = message(Kind::Ack);
      ack.sequence = m.sequence; ack.requesterBoot = m.boot; ack.result = result;
      enqueue(sender, ack);
    } else if (m.kind == Kind::Ack && operation.active && m.requesterBoot == bootId) {
      for (size_t i = 0; i < operation.count; ++i) {
        PendingCommand &pending = operation.commands[i];
        if (pending.target == sender && pending.message.sequence == m.sequence) {
          pending.done = true; pending.result = m.result;
        }
      }
    }
  }
}

void processCommands() {
  if (!operation.active) return;
  bool complete = true;
  for (size_t i = 0; i < operation.count; ++i) {
    PendingCommand &pending = operation.commands[i];
    if (pending.done) continue;
    if (pending.target == selfId) {
      pending.result = applyCommand(pending.message); pending.done = true;
    } else if (millis() - operation.started >= 5000) {
      pending.result = Result::Timeout; pending.done = true;
    } else {
      complete = false;
      if (!pending.sentAt || millis() - pending.sentAt >= 300) {
        if (enqueue(pending.target, pending.message)) pending.sentAt = millis();
      }
    }
  }
  if (complete) operation.active = false;
}

void saveHomeCredentials() {
  homeSsid = WiFi.SSID(); homePassword = WiFi.psk();
  prefs.putString("ssid", homeSsid); prefs.putString("password", homePassword);
}

void stopPortal() {
  if (wifiManager.getConfigPortalActive()) wifiManager.stopConfigPortal();
  if (wifiManager.getWebPortalActive()) wifiManager.stopWebPortal();
}

String gatewaySsid(uint64_t id, Preference mode) {
  return "Scales-" + String(kit) + "-" + idText(id) + "-" +
         String(mode == Preference::PreferGateway ? 1 : 0);
}

void changeRole(Role role) {
  ledGateway.store(role == Role::Gateway);
  Serial.printf("Network role=%u kit=%s gateway=%s\n", static_cast<unsigned>(role), kit,
                idText(election->gateway()).c_str());
  if (mdnsActive) MDNS.end();
  mdnsActive = false; lastMdnsAttempt = 0;
  stopPortal();
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  xQueueReset(transmitQueue);
  if (operation.active) {
    for (size_t i = 0; i < operation.count; ++i) {
      if (!operation.commands[i].done) {
        operation.commands[i].done = true;
        operation.commands[i].result = Result::Timeout;
      }
    }
    operation.active = false;
  }
  for (size_t i = 1; i < 4; ++i) nodes[i] = Node{};
  if (role == Role::Gateway) {
    hotspot = gatewaySsid(selfId, preference);
    wifiManager.startConfigPortal(hotspot.c_str(), AP_PASSWORD);
    // Reserve room for three scales plus browser clients.
    WiFi.softAP(hotspot.c_str(), AP_PASSWORD, WiFi.channel(), false, 8);
    if (homeSsid.length()) WiFi.begin(homeSsid.c_str(), homePassword.c_str());
    lastHomeRetry = millis();
    Serial.printf("Gateway: %s | http://192.168.4.1/\n", hotspot.c_str());
  } else {
    wifiManager.startWebPortal();
    if (role == Role::Member) {
      // The discovered SSID is recorded by processDiscovery before this call.
      WiFi.begin(hotspot.c_str(), AP_PASSWORD);
      lastHomeRetry = millis();
      Serial.printf("Joining gateway %s SSID=%s\n", idText(election->gateway()).c_str(), hotspot.c_str());
    } else Serial.println("Searching for a kit gateway");
  }
  WiFi.setSleep(false);
  wasHomeConnected = false;
}

bool parseGateway(const String &ssid, Candidate &candidate) {
  String prefix = "Scales-" + String(kit) + "-";
  if (!ssid.startsWith(prefix) || ssid.length() != prefix.length() + 14) return false;
  String id = ssid.substring(prefix.length(), prefix.length() + 12);
  for (char c : id) if (!isxdigit(static_cast<unsigned char>(c))) return false;
  if (ssid[prefix.length() + 12] != '-') return false;
  char mode = ssid[prefix.length() + 13];
  if (mode != '0' && mode != '1') return false;
  candidate.id = strtoull(id.c_str(), nullptr, 16);
  candidate.preference = mode == '1' ? Preference::PreferGateway : Preference::Auto;
  return candidate.id != 0;
}

void processDiscovery() {
  uint32_t now = millis();
  Role before = election->role();
  if (scanRunning) {
    int count = WiFi.scanComplete();
    if (count != WIFI_SCAN_RUNNING) {
      Serial.printf("Discovery scan result=%d elapsed=%lu ms\n", count,
                    static_cast<unsigned long>(now - scanStartedAt));
      if (count < 0) esp_wifi_scan_stop();
      Candidate candidates[16];
      String ssids[16];
      size_t found = 0;
      for (int i = 0; i < count && found < 16; ++i) {
        Candidate candidate;
        String ssid = WiFi.SSID(i);
        bool matches = parseGateway(ssid, candidate);
        if (ssid.startsWith("Scales-"))
          Serial.printf("Discovery SSID=%s channel=%d RSSI=%d matches=%d\n",
                        ssid.c_str(), WiFi.channel(i), WiFi.RSSI(i), matches);
        if (matches && candidate.id != selfId) {
          candidates[found] = candidate; ssids[found++] = WiFi.SSID(i);
        }
      }
      election->scanned(candidates, found, now);
      if (election->role() == Role::Member && before != Role::Member) {
        for (size_t i = 0; i < found; ++i)
          if (candidates[i].id == election->gateway()) hotspot = ssids[i];
      }
      WiFi.scanDelete(); scanRunning = false; lastScan = now;
    }
  }
  // Allow a scan already in progress to finish before declaring ourselves host.
  if (!scanRunning) election->tick(now);
  if (before != election->role()) changeRole(election->role());
  uint32_t interval = election->role() == Role::Gateway ? 20000 : 1000;
  bool deferScan = election->role() == Role::Gateway && liveSocket.connectedClients() > 0;
  if (!scanRunning && !deferScan && election->role() != Role::Member && now - lastScan >= interval) {
    // Arduino derives the scan timeout from this dwell time. 120 ms allowed
    // only 2.4 seconds, which can expire before the full scan completes.
    int result = WiFi.scanNetworks(true, false, false, 300);
    Serial.printf("Discovery scan start=%d role=%u kit=%s channel=%d live=%d\n", result,
                  static_cast<unsigned>(election->role()), kit, WiFi.channel(),
                  liveSocket.connectedClients());
    if (result == WIFI_SCAN_RUNNING) {
      scanStartedAt = now;
      scanRunning = true;
    } else lastScan = now;
  }
}

void processNetwork() {
  wifiManager.process();
  Role before = election->role();
  processReceives();
  if (before != election->role()) {
    if (scanRunning) {
      esp_wifi_scan_stop(); WiFi.scanDelete(); scanRunning = false;
    }
    changeRole(election->role());
  }
  processDiscovery();
  if (election->role() == Role::Gateway) {
    if (!wifiManager.getConfigPortalActive())
      wifiManager.startConfigPortal(hotspot.c_str(), AP_PASSWORD);
    if (!mdnsActive && (!lastMdnsAttempt || millis() - lastMdnsAttempt >= 10000)) {
      lastMdnsAttempt = millis();
      String host = kit; host.toLowerCase();
      mdnsActive = MDNS.begin(host);
      if (mdnsActive) {
        MDNS.setInstanceName("Race scales " + String(kit));
        MDNS.addService("http", "tcp", 80);
        MDNS.addServiceTxt("http", "tcp", "path", "/");
        MDNS.addServiceTxt("http", "tcp", "id", idText(selfId));
        Serial.printf("Kit address: http://%s.local/\n", host.c_str());
      } else MDNS.end();
    }
    bool connected = WiFi.status() == WL_CONNECTED;
    if (connected && !wasHomeConnected) {
      Serial.printf("Home WiFi: http://%s/ | channel %d\n",
                    WiFi.localIP().toString().c_str(), WiFi.channel());
    }
    wasHomeConnected = connected;
    if (!connected && homeSsid.length() && millis() - lastHomeRetry >= 30000) {
      lastHomeRetry = millis();
      WiFi.begin(homeSsid.c_str(), homePassword.c_str());
    }
  } else if (election->role() == Role::Member && WiFi.status() != WL_CONNECTED &&
             millis() - lastHomeRetry >= 10000) {
    // ESP-NOW beacons can arrive even after association fails. They keep the
    // gateway alive, so retry Wi-Fi independently of the heartbeat timeout.
    lastHomeRetry = millis();
    Serial.printf("Retrying gateway WiFi SSID=%s\n", hotspot.c_str());
    WiFi.begin(hotspot.c_str(), AP_PASSWORD);
  }
  if (millis() - lastNetworkSend >= 250) {
    lastNetworkSend = millis();
    if (election->role() == Role::Gateway) {
      enqueue(nodeId(Broadcast), message(Kind::Beacon));
    } else if (election->role() == Role::Member) {
      // ESP-NOW does not require Wi-Fi association. Keep sending on the current
      // radio channel while association retries; gateway beacons maintain membership.
      enqueue(election->gateway(), nodes[0].reading);
    }
  }
  processCommands();
  processTransmits();
  static uint32_t lastReport = 0;
  if (millis() - lastReport >= 10000) {
    lastReport = millis();
    Serial.printf("Network role=%u wifi=%d SSID=%s IP=%s channel=%d tx_power=%.1f dBm rx=%lu rejected=%lu tx_error=%s delivery=%d busy=%d\n",
                  static_cast<unsigned>(election->role()), WiFi.status(), WiFi.SSID().c_str(),
                  WiFi.localIP().toString().c_str(), WiFi.channel(), static_cast<int>(WiFi.getTxPower()) / 4.0,
                  static_cast<unsigned long>(receivedFrames), static_cast<unsigned long>(rejectedFrames),
                  esp_err_to_name(lastSendError), lastDeliveryStatus.load(), transmitBusy.load());
  }
}

String dataJson() {
  float weights[5] = {}, total = 0;
  uint8_t counts[5] = {}, onlineCount = 0, validCount = 0;
  bool balance = true;
  String list = "[";
  bool first = true;
  for (const Node &node : nodes) {
    if (!node.id) continue;
    bool isOnline = online(node), isReady = ready(node);
    bool calibrated = node.reading.flags & Calibrated;
    uint8_t metadata = node.reading.readingMetadataVersion;
    bool haveCalibration = metadata >= 1 && metadata <= 4;
    if (isOnline) ++onlineCount;
    if (isReady && calibrated) { total += node.reading.weight; ++validCount; }
    unsigned c = static_cast<unsigned>(node.reading.corner);
    if (isReady && calibrated && c) { weights[c] += node.reading.weight; ++counts[c]; }
    if (!first) list += ',';
    first = false;
    list += "{\"id\":\"" + idText(node.id) + "\",\"corner\":" + String(c) +
            ",\"boot_id\":" + String(node.reading.boot) +
            ",\"update_available\":" + String(node.reading.preference != Preference::ScaleOnly &&
                                               (node.id == selfId || node.reading.ip) ? "true" : "false") +
            ",\"wifi_host\":" + String(node.id == selfId && election->role() == Role::Gateway ? "true" : "false") +
            ",\"preference\":" + String(static_cast<unsigned>(node.reading.preference)) +
            ",\"online\":" + String(isOnline ? "true" : "false") +
            ",\"ready\":" + String(isReady ? "true" : "false") +
            ",\"calibrated\":" + String(calibrated ? "true" : "false") +
            ",\"firmware_version\":\"" + String(node.reading.firmwareVersion[0] ? node.reading.firmwareVersion : "unknown") + "\"" +
            ",\"calibration\":" + (haveCalibration ? String(node.reading.calibration, 6) : String("null")) +
            ",\"offset\":" + (haveCalibration ? String(node.reading.zeroOffset, 1) : String("null")) +
            ",\"battery_voltage\":" + (metadata == 4 ? String(node.reading.batteryMillivolts / 1000.0f, 2) : String("null")) +
            ",\"weight\":" + String(node.reading.weight, 1) +
            ",\"raw\":" + String(node.reading.raw, 0) + "}";
  }
  list += ']';
  for (int i = 1; i <= 4; ++i) if (counts[i] != 1) balance = false;
  balance = balance && total > 0;
  String json = "{\"uptime_ms\":" + String(millis()) + ",\"firmware_version\":\"" SCALE_FIRMWARE_VERSION "\",\"nodes\":" + list + ",\"kit\":\"" + String(kit) +
                "\",\"role\":\"" + String(election->role() == Role::Gateway ? "gateway" :
                                            election->role() == Role::Member ? "member" : "searching") +
                "\",\"online_count\":" + String(onlineCount) +
                ",\"valid_count\":" + String(validCount) +
                ",\"balance_available\":" + String(balance ? "true" : "false") + ',';
  const char *names[] = {"", "lf", "rf", "lr", "rr"};
  for (int i = 1; i <= 4; ++i) {
    bool isOnline = false, isLocked = false;
    for (const Node &node : nodes) {
      if (static_cast<int>(node.reading.corner) == i && ready(node)) {
        isOnline = true; isLocked = node.reading.flags & Locked;
      }
    }
    json += "\"" + String(names[i]) + "\":" + String(counts[i] == 1 ? weights[i] : 0, 1) +
            ",\"" + names[i] + "_online\":" + String(isOnline ? "true" : "false") +
            ",\"" + names[i] + "_locked\":" + String(isLocked ? "true" : "false") + ',';
  }
  float front = weights[1] + weights[2], rear = weights[3] + weights[4];
  float left = weights[1] + weights[3], right = weights[2] + weights[4];
  json += "\"total\":" + String(total, 1) + ",\"front\":" + String(front, 1) +
          ",\"rear\":" + String(rear, 1) + ",\"left\":" + String(left, 1) +
          ",\"right\":" + String(right, 1);
  float numerators[] = {front, rear, left, right, weights[1] + weights[4]};
  const char *percentages[] = {"frontpct", "rearpct", "leftpct", "rightpct", "cross"};
  for (int i = 0; i < 5; ++i)
    json += ",\"" + String(percentages[i]) + "\":" + String(balance ? numerators[i] * 100 / total : 0, 1);
  return json + '}';
}

const char *resultText(Result result) {
  switch (result) {
    case Result::Ok: return "OK";
    case Result::Offline: return "No fresh load-cell reading";
    case Result::Invalid: return "Invalid calibration reading or settings";
    case Result::Stale: return "An older command was rejected";
    default: return "Scale did not acknowledge the command";
  }
}

void handleOperation() {
  if (!operation.id || server().arg("id") != String(operation.id)) {
    server().send(404, "application/json", "{\"error\":\"Unknown operation\"}"); return;
  }
  String json = "{\"pending\":" + String(operation.active ? "true" : "false") + ",\"results\":[";
  for (size_t i = 0; i < operation.count; ++i) {
    if (i) json += ',';
    const PendingCommand &command = operation.commands[i];
    json += "{\"id\":\"" + idText(command.target) + "\",\"ok\":" +
            String(command.done && command.result == Result::Ok ? "true" : "false") +
            ",\"message\":\"" + String(resultText(command.result)) + "\"}";
  }
  server().send(200, "application/json", json + "]}");
}

Node *requestedNode() {
  String target = server().arg("id");
  if (target.length()) {
    for (Node &node : nodes) if (node.id && idText(node.id).equalsIgnoreCase(target)) return &node;
    return nullptr;
  }
  String pad = server().arg("pad"); pad.toUpperCase(); pad.trim();
  Node *found = nullptr;
  for (Node &node : nodes) {
    if (!online(node)) continue;
    if (!pad.length() || pad == CornerNames[static_cast<unsigned>(node.reading.corner)] ||
        idText(node.id).equalsIgnoreCase(pad)) {
      if (found) return nullptr;
      found = &node;
    }
  }
  return found;
}

bool beginOperation() {
  if (operation.active || restartAt) {
    server().send(409, "application/json", "{\"error\":\"Another change is still in progress\"}");
    return false;
  }
  operation = Operation{};
  operation.id = nextSequence(); operation.started = millis(); operation.active = true;
  return true;
}

void accepted() {
  server().send(202, "application/json", "{\"operation\":" + String(operation.id) + "}");
}

void handleTare() {
  if (!beginOperation()) return;
  for (const Node &node : nodes) {
    if (!ready(node)) continue;
    PendingCommand &command = operation.commands[operation.count++];
    command.target = node.id; command.message = message(Kind::Command);
    command.message.command = Command::Tare;
  }
  if (!operation.count) {
    operation.active = false;
    server().send(400, "application/json", "{\"error\":\"No scales have fresh readings\"}");
  } else accepted();
}

void handleCalibrate() {
  Node *node = requestedNode();
  char *end;
  String weight = server().arg("weight");
  float known = strtof(weight.c_str(), &end);
  if (!node || !ready(*node) || end == weight.c_str() || *end ||
      !std::isfinite(known) || known <= 0) {
    server().send(400, "application/json", "{\"error\":\"Choose one online scale and a positive known weight\"}");
    return;
  }
  if (!beginOperation()) return;
  operation.count = 1;
  PendingCommand &command = operation.commands[0];
  command.target = node->id; command.message = message(Kind::Command);
  command.message.command = Command::Calibrate; command.message.knownWeight = known;
  accepted();
}

bool option(const String &value, unsigned max, uint8_t &out) {
  if (value.length() != 1 || value[0] < '0' || value[0] > '0' + max) return false;
  out = value[0] - '0'; return true;
}

void handleConfigure() {
  Node *node = requestedNode();
  String newKit = server().arg("kit"); newKit.trim(); newKit.toUpperCase();
  uint8_t newCorner, newPreference;
  if (!node || !online(*node) || newKit.length() > 8 || !validKit(newKit.c_str()) ||
      !option(server().arg("corner"), 4, newCorner) ||
      !option(server().arg("mode"), 2, newPreference)) {
    server().send(400, "application/json", "{\"error\":\"Invalid scale, kit code, corner, or role\"}"); return;
  }
  if (!beginOperation()) return;
  operation.count = 1;
  PendingCommand &command = operation.commands[0];
  command.target = node->id; command.message = message(Kind::Command);
  command.message.command = Command::Configure;
  newKit.toCharArray(command.message.configKit, sizeof(command.message.configKit));
  command.message.configCorner = static_cast<Corner>(newCorner);
  command.message.configPreference = static_cast<Preference>(newPreference);
  accepted();
}

void handleSettings() {
  String html = "<!doctype html><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>Scale settings</title><style>body{font:16px Arial;background:#111;color:#eee;max-width:640px;"
    "margin:30px auto;padding:15px}a{color:#22c55e}fieldset{margin:15px 0}label{display:block;margin:12px 0}"
    "input,select,button{padding:8px;font-size:16px}button{cursor:pointer}</style><h1>Scale settings</h1>"
    "<p><a href='/'>Race scales</a> · <a href='/wifi'>Wi-Fi setup</a> · "
    "<a href='/diagnostics'>HX711 diagnostics</a></p>"
    "<p>Assign corners and role preferences here. All scales in a kit must use the same kit code "
    "(1–8 letters/numbers). Settings stay with each scale. Saving restarts the selected scale.</p>";
  for (const Node &node : nodes) {
    if (!node.id) continue;
    html += "<form class='scale-config'><fieldset><legend>" + idText(node.id) + (node.id == selfId ? " (this scale)" : "") +
            " / v" + (node.reading.firmwareVersion[0] ? node.reading.firmwareVersion : "unknown") +
            (online(node) ? "" : " — offline") + "</legend><input type='hidden' name='id' value='" +
            idText(node.id) + "'><label>Kit code <input name='kit' maxlength='8' required pattern='[A-Za-z0-9]{1,8}' value='" +
            String(kit) + "'></label><label>Corner <select name='corner'>";
    for (int c = 0; c < 5; ++c)
      html += "<option value='" + String(c) + "'" + (static_cast<int>(node.reading.corner) == c ? " selected" : "") +
              ">" + CornerNames[c] + "</option>";
    html += "</select></label><label>Role <select name='mode'>";
    for (int p = 0; p < 3; ++p)
      html += "<option value='" + String(p) + "'" + (static_cast<int>(node.reading.preference) == p ? " selected" : "") +
              ">" + PreferenceNames[p] + "</option>";
    html += "</select></label><button" + String(online(node) ? "" : " disabled") + ">Save settings</button>";
    if (node.reading.preference == Preference::ScaleOnly) {
      html += " <span>Firmware updates are not available in Scale only mode. Select Auto or Prefer gateway to update.</span>";
    } else if (online(node)) {
      String url = node.id == selfId ? String("/update") : "/member-update/" + idText(node.id);
      html += " <a href='" + url + "'>Update this scale's firmware</a>";
    }
    html += "</fieldset></form>";
  }
  html += "<p>Wi-Fi credentials are local to each board. Configure a board while it is the gateway. "
          "To change the whole kit code, update the other scales first and the gateway last. "
          "Member firmware uploads are relayed through this gateway; you can stay on your current Wi-Fi.</p><p id='status'></p>"
          "<script>document.querySelectorAll('form.scale-config').forEach(f=>f.onsubmit=async e=>{e.preventDefault();"
          "const s=document.getElementById('status');try{let r=await fetch('/node',{method:'POST',body:new URLSearchParams(new FormData(f))});"
          "let d=await r.json();if(!r.ok)throw Error(d.error);s.textContent='Saving…';"
          "for(let i=0;i<30;i++){await new Promise(r=>setTimeout(r,250));r=await fetch('/operation?id='+d.operation);"
          "let a=await r.json();if(!a.pending){let bad=a.results.filter(x=>!x.ok);if(bad.length)throw Error(bad.map(x=>x.message).join('; '));"
          "s.textContent='Saved. The scale is restarting; reconnect to the gateway if needed.';return}}throw Error('Timed out')"
          "}catch(e){s.textContent=e.message}})</script>";
  if (election->role() == Role::Gateway) html += FPSTR(KIT_UPDATE);
  server().sendHeader("Cache-Control", "no-store");
  server().send(200, "text/html", html);
}

struct MemberUpload {
  WiFiClient client;
  String footer, error;
  size_t expected = 0, received = 0;
  bool complete = false;
} memberUpload;

Node *requestedUpdateNode(bool requireOnline = true) {
  if (election->role() != Role::Gateway) return nullptr;
  String id = server().pathArg(0);
  for (Node &node : nodes) {
    if (node.id != selfId && (!requireOnline || online(node)) && idText(node.id).equalsIgnoreCase(id))
      return &node;
  }
  return nullptr;
}

void handleMemberUpdatePage() {
  Node *node = requestedUpdateNode();
  if (!node) {
    server().send(404, "text/plain", "Scale unavailable. Open Scale settings on the current gateway.");
    return;
  }
  if (node->reading.preference == Preference::ScaleOnly) {
    server().send(403, "text/plain", "Firmware updates are not available in Scale only mode. Select Auto or Prefer gateway in Scale settings.");
    return;
  }
  String html = FPSTR(MEMBER_UPDATE);
  html.replace("%TARGET%", String(CornerNames[static_cast<unsigned>(node->reading.corner)]) + " / " +
               idText(node->id) + " / v" + (node->reading.firmwareVersion[0] ? node->reading.firmwareVersion : "unknown"));
  server().sendHeader("Cache-Control", "no-store");
  server().send(200, "text/html", html);
}

void relayHeartbeat() {
  if (election->role() != Role::Gateway) return;
  // WebServer handles uploads synchronously. Keep the gateway visible while it
  // relays a file, without recursively entering WiFiManager's HTTP handler.
  if (millis() - lastNetworkSend >= 250) {
    lastNetworkSend = millis();
    enqueue(nodeId(Broadcast), message(Kind::Beacon));
  }
  processTransmits();
}

bool relayWrite(const uint8_t *data, size_t length) {
  while (length) {
    relayHeartbeat();
    size_t sent = memberUpload.client.write(data, length);
    if (!sent) {
      memberUpload.error = "Connection to the selected scale failed during transfer.";
      memberUpload.client.stop();
      return false;
    }
    data += sent; length -= sent;
    yield();
  }
  return true;
}

void handleMemberUpload() {
  relayHeartbeat();
  // WebServer shares the callback between raw and multipart bodies. Reject
  // other content types before accessing raw(), which is absent for multipart.
  if (server().header("Content-Type") != "application/octet-stream") {
    memberUpload.error = "Upload the application image as application/octet-stream.";
    memberUpload.complete = false; memberUpload.client.stop(); return;
  }
  HTTPRaw &upload = server().raw();
  if (upload.status == RAW_START) {
    memberUpload.client.stop();
    memberUpload.error = ""; memberUpload.footer = "";
    memberUpload.received = 0; memberUpload.complete = false;
    char *end;
    String size = server().header("Content-Length");
    memberUpload.expected = strtoul(size.c_str(), &end, 10);
    const esp_partition_t *slot = esp_ota_get_next_update_partition(nullptr);
    // HTTP parsing can stall the main loop past the live-reading timeout.
    // Use the saved address here; the HTTP device-ID probe below proves reachability.
    Node *node = requestedUpdateNode(false);
    if (!node) {
      memberUpload.error = "Selected scale is unknown or this board is no longer the gateway."; return;
    }
    if (node->reading.preference == Preference::ScaleOnly) {
      memberUpload.error = "Firmware updates are not available in Scale only mode. Select Auto or Prefer gateway."; return;
    }
    if (operation.active) {
      memberUpload.error = "A scale command is still in progress. Wait for it to finish before updating."; return;
    }
    if (restartAt) {
      memberUpload.error = "The gateway is restarting. Reconnect before updating."; return;
    }
    if (end == size.c_str() || *end || memberUpload.expected < 24 ||
        !slot || memberUpload.expected > slot->size) {
      memberUpload.error = "Choose a valid ESP32-S3 application firmware.bin file."; return;
    }
    IPAddress ip(node->reading.ip), ap = WiFi.softAPIP(), mask = WiFi.softAPSubnetMask();
    if (!node->reading.ip || (static_cast<uint32_t>(ip) & static_cast<uint32_t>(mask)) !=
        (static_cast<uint32_t>(ap) & static_cast<uint32_t>(mask)) || ip == ap) {
      memberUpload.error = "Selected scale has no gateway Wi-Fi connection. ESP-NOW readings alone do not enable OTA. Wait for Wi-Fi to reconnect."; return;
    }
    if (scanRunning) {
      esp_wifi_scan_stop(); WiFi.scanDelete(); scanRunning = false;
    }
    // Check identity before flashing: a reboot can assign a member IP to a different scale.
    WiFiClient probeClient;
    HTTPClient probe;
    probe.setConnectTimeout(3000); probe.setTimeout(3000);
    probe.begin(probeClient, "http://" + ip.toString() + "/diagnostics/data");
    int status = probe.GET();
    String identity = status == 200 ? probe.getString() : String("");
    probe.end();
    if (identity.indexOf("\"id\":\"" + idText(node->id) + "\"") < 0) {
      memberUpload.error = "Cannot verify the selected scale. Wait for it to reconnect and retry."; return;
    }
    memberUpload.client.setTimeout(5000);
    if (!memberUpload.client.connect(ip, 80, 3000)) {
      memberUpload.error = "Cannot connect to the selected scale's updater."; return;
    }
    String boundary = "RaceScale" + String(esp_random(), HEX);
    String prefix = "--" + boundary + "\r\nContent-Disposition: form-data; name=\"update\"; filename=\"firmware.bin\"\r\n"
                    "Content-Type: application/octet-stream\r\n\r\n";
    memberUpload.footer = "\r\n--" + boundary + "--\r\n";
    String headers = "POST /u HTTP/1.1\r\nHost: " + ip.toString() + "\r\nConnection: close\r\n"
      "Content-Type: multipart/form-data; boundary=" + boundary + "\r\nContent-Length: " +
      String(memberUpload.expected + prefix.length() + memberUpload.footer.length()) + "\r\n\r\n";
    if (relayWrite(reinterpret_cast<const uint8_t *>(headers.c_str()), headers.length()))
      relayWrite(reinterpret_cast<const uint8_t *>(prefix.c_str()), prefix.length());
  } else if (upload.status == RAW_WRITE && !memberUpload.error.length()) {
    if ((!memberUpload.received && upload.buf[0] != 0xe9) ||
        upload.currentSize > memberUpload.expected - memberUpload.received) {
      memberUpload.error = "Invalid application image or upload length.";
      memberUpload.client.stop(); return;
    }
    if (relayWrite(upload.buf, upload.currentSize)) memberUpload.received += upload.currentSize;
  } else if (upload.status == RAW_END && !memberUpload.error.length()) {
    if (memberUpload.received != memberUpload.expected) {
      memberUpload.error = "Incomplete firmware upload.";
    } else if (relayWrite(reinterpret_cast<const uint8_t *>(memberUpload.footer.c_str()), memberUpload.footer.length())) {
      String response;
      uint32_t started = millis();
      while ((memberUpload.client.connected() || memberUpload.client.available()) &&
             millis() - started < 5000 && response.length() < 16384) {
        char buffer[128];
        int count = memberUpload.client.read(reinterpret_cast<uint8_t *>(buffer), sizeof(buffer));
        if (count > 0) response.concat(buffer, count);
        relayHeartbeat(); delay(1);
      }
      memberUpload.complete = (response.startsWith("HTTP/1.1 200 ") || response.startsWith("HTTP/1.0 200 ")) &&
                              response.indexOf("Update successful.") >= 0;
      if (!memberUpload.complete) memberUpload.error = "The selected scale did not confirm the update. Check its version before retrying.";
    }
    memberUpload.client.stop(); lastScan = millis();
  } else if (upload.status == RAW_ABORTED) {
    memberUpload.error = "Upload cancelled or connection lost.";
    memberUpload.client.stop(); lastScan = millis();
  }
}

void handleMemberUploadDone() {
  if (memberUpload.complete) server().send(200, "application/json", "{\"ok\":true}");
  else server().send(502, "application/json", "{\"ok\":false,\"message\":\"" +
                    (memberUpload.error.length() ? memberUpload.error : String("Upload failed.")) + "\"}");
}

void handleDiagnosticsMode() {
  if (server().arg("dual") != "0" && server().arg("dual") != "1") {
    server().send(400, "application/json", "{\"error\":\"Specify dual=0 or dual=1\"}"); return;
  }
  if (server().arg("dual") == "1" && (operation.active || restartAt)) {
    server().send(409, "application/json", "{\"error\":\"Wait for the current command to finish\"}"); return;
  }
  dualDiagnostics = server().arg("dual") == "1";
  diagnosticsRenewed = millis();
  if (dualDiagnostics) {
    haveReading = false; locked = false;
    nodes[0].reading.flags &= ~(Ready | Locked);
  }
  server().send(200, "application/json", "{\"ok\":true}");
}

String diagnosticsJson() {
  uint32_t now = millis();
  const esp_partition_t *running = esp_ota_get_running_partition();
  String json = "{\"uptime_ms\":" + String(now) + ",\"id\":\"" + idText(selfId) + "\",\"dt_pin\":" + String(HX_DT) +
    ",\"boot_id\":" + String(bootId) +
    ",\"cpu_mhz\":" + String(getCpuFrequencyMhz()) +
    ",\"firmware_version\":\"" SCALE_FIRMWARE_VERSION "\"" +
    ",\"firmware_build\":\"" + String(__DATE__ " " __TIME__) + "\",\"app_partition\":\"" +
    String(running ? running->label : "unknown") + "\"" +
    ",\"sck_pin\":" + String(HX_SCK) + ",\"dt_level\":" + String(digitalRead(HX_DT)) +
    ",\"responding\":" + String(conversionCount && now - lastSample < 2000 ? "true" : "false") +
    ",\"conversions\":" + String(conversionCount) + ",\"rate_hz\":" + String(conversionHz, 1) + ",\"last_conversion_ms\":" +
    (conversionCount ? String(now - lastSample) : String("null")) +
    ",\"dual\":" + String(dualDiagnostics ? "true" : "false") +
    ",\"weighing_ready\":" + String(ready(nodes[0]) ? "true" : "false") +
    ",\"average_a\":" + (haveReading ? String(rawReading, 1) : String("null")) +
    ",\"offset\":" + String(offset, 1) + ",\"calibration\":" + String(calibration, 6) +
    ",\"channels\":[";
  for (size_t i = 0; i < 2; ++i) {
    if (i) json += ',';
    const RawChannel &channel = rawChannels[i];
    json += "{\"name\":\"" + String(i ? "B" : "A") + "\",\"gain\":" + String(i ? 32 : 128) +
      ",\"raw\":" + (channel.count ? String(channel.value) : String("null")) +
      ",\"age_ms\":" + (channel.count ? String(now - channel.sampledAt) : String("null")) +
      ",\"samples\":" + String(channel.count) + ",\"saturated\":" +
      String(channel.count && (channel.value == -8388608L || channel.value == 8388607L) ? "true" : "false") + "}";
  }
  return json + "]}";
}

void onLiveSocket(uint8_t client, WStype_t type, uint8_t *payload, size_t length) {
  if (type == WStype_DISCONNECTED) liveStreams[client] = 0;
  else if (type == WStype_CONNECTED) {
    if (length == 7 && !memcmp(payload, "/scales", 7)) liveStreams[client] = 1;
    else if (length == 12 && !memcmp(payload, "/diagnostics", 12)) liveStreams[client] = 2;
    else liveSocket.disconnect(client);
  } else if (type == WStype_TEXT && liveStreams[client] == 2 && dualDiagnostics &&
             length == 18 && !memcmp(payload, "diagnostics-active", 18)) {
    diagnosticsRenewed = millis();
  }
}

void pushLiveUpdates() {
  static uint32_t lastPush = 0;
  if (millis() - lastPush < 200) return;
  lastPush = millis();
  String dashboardData, diagnosticData;
  for (uint8_t client = 0; client < WEBSOCKETS_SERVER_CLIENT_MAX; ++client) {
    if (!liveStreams[client] || !liveSocket.clientIsConnected(client)) continue;
    String &data = liveStreams[client] == 1 ? dashboardData : diagnosticData;
    if (!data.length()) data = liveStreams[client] == 1 ? dataJson() : diagnosticsJson();
    if (!liveSocket.sendTXT(client, data)) liveSocket.disconnect(client);
  }
}

void registerRoutes() {
  const char *headers[] = {"Content-Length", "Content-Type"};
  server().collectHeaders(headers, 2);
  server().on(UriBraces("/member-update/{}"), HTTP_GET, handleMemberUpdatePage);
  server().on(UriBraces("/member-update/{}"), HTTP_POST, handleMemberUploadDone, handleMemberUpload);
  // WiFiManager installs its menu route after this callback. Our root handler
  // takes precedence; Wi-Fi provisioning remains available through /wifi.
  auto dashboard = []() {
    if (election->role() == Role::Member) {
      server().sendHeader("Location", "http://192.168.4.1/");
      server().send(302, "text/plain", "Open the gateway dashboard");
    } else {
      server().sendHeader("Cache-Control", "no-store");
      server().send_P(200, "text/html", DASHBOARD);
    }
  };
  server().on("/", HTTP_GET, dashboard);
  server().on("/scales", HTTP_GET, dashboard);
  server().on("/data", []() {
    server().sendHeader("Cache-Control", "no-store");
    server().send(200, "application/json", dataJson());
  });
  server().on("/settings", handleSettings);
  server().on("/live.js", []() {
    server().sendHeader("Cache-Control", "no-store");
    server().send_P(200, "application/javascript", LIVE_UPDATES);
  });
  server().on("/diagnostics", []() {
    server().sendHeader("Cache-Control", "no-store");
    server().send_P(200, "text/html", DIAGNOSTICS);
  });
  server().on("/diagnostics/data", HTTP_GET, []() {
    server().sendHeader("Cache-Control", "no-store");
    server().send(200, "application/json", diagnosticsJson());
  });
  server().on("/diagnostics/mode", HTTP_POST, handleDiagnosticsMode);
  server().on("/operation", handleOperation);
  server().on("/tare", HTTP_POST, handleTare);
  server().on("/calibrate", HTTP_POST, handleCalibrate);
  server().on("/node", HTTP_POST, handleConfigure);
  if (election->role() != Role::Gateway) {
    auto gatewayOnly = []() { server().send(403, "text/plain", "Configure Wi-Fi while this scale is the gateway (power it alone)."); };
    server().on("/wifi", gatewayOnly);
    server().on("/wifinoscan", gatewayOnly);
    server().on("/wifisave", gatewayOnly);
  }
}

void updateStatusLed(TimerHandle_t) {
  // The 100 ms software timer keeps blinking through blocking HTTP uploads.
  static uint8_t phase = 0;
  if (phase == 0) {
    bool gateway = ledGateway.load();
    neopixelWrite(STATUS_RGB_PIN, 0, gateway ? 0 : 16, gateway ? 16 : 0);
  } else if (phase == 1) neopixelWrite(STATUS_RGB_PIN, 0, 0, 0);
  phase = (phase + 1) % 10;
}

void loadConfiguration() {
  prefs.begin("node", false);
  String savedKit = prefs.getString("kit", "SCALE");
  if (savedKit.length() <= 8 && validKit(savedKit.c_str())) savedKit.toCharArray(kit, sizeof(kit));
  corner = static_cast<Corner>(prefs.getUChar("corner", 0));
  preference = static_cast<Preference>(prefs.getUChar("mode", 0));
  if (corner > Corner::RR) corner = Corner::Unassigned;
  if (preference > Preference::ScaleOnly) preference = Preference::Auto;
  calibration = prefs.getFloat("cal", 0);
  haveOffset = prefs.isKey("offset"); offset = prefs.getFloat("offset", 0);
  if (!prefs.isKey("migrated")) {
    Preferences legacy;
    if (legacy.begin("scales", true)) {
      // Historical NVS keys must retain their original spelling for migration.
      float oldCal = legacy.getFloat("FL_cal", 0);
      if (std::isfinite(oldCal) && fabsf(oldCal) > 0.0001f && oldCal != 1.0f) {
        calibration = oldCal; offset = legacy.getFloat("FL_offset", 0);
        haveOffset = true; corner = Corner::LF;
        prefs.putFloat("cal", calibration); prefs.putFloat("offset", offset);
        prefs.putUChar("corner", static_cast<uint8_t>(corner));
      }
      legacy.end();
    }
    prefs.putBool("migrated", true);
  }
  if (!std::isfinite(calibration)) calibration = 0;
  if (!std::isfinite(offset)) { offset = 0; haveOffset = false; }
  homeSsid = prefs.getString("ssid", ""); homePassword = prefs.getString("password", "");
  if (!prefs.isKey("ssid")) {
    wifi_config_t saved{};
    if (esp_wifi_get_config(WIFI_IF_STA, &saved) == ESP_OK && saved.sta.ssid[0]) {
      char ssid[33]{}, password[65]{};
      memcpy(ssid, saved.sta.ssid, 32); memcpy(password, saved.sta.password, 64);
      homeSsid = ssid; homePassword = password;
      prefs.putString("ssid", homeSsid); prefs.putString("password", homePassword);
    }
  }
}

void setup() {
  Serial.begin(115200);
  neopixelWrite(STATUS_RGB_PIN, 0, 0, 0);
  TimerHandle_t ledTimer = xTimerCreate("scale-led", pdMS_TO_TICKS(100), pdTRUE, nullptr, updateStatusLed);
  if (!ledTimer || xTimerStart(ledTimer, 0) != pdPASS) Serial.println("Status LED timer failed to start");
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    if (event == ARDUINO_EVENT_WIFI_STA_START) {
      // Request 12 dBm in quarter-dBm units; log the driver's reported limit.
      // The driver must be started before setting transmit power.
      bool powerSet = esp_wifi_set_max_tx_power(12 * 4) == ESP_OK;
      Serial.printf("Radio power set=%d actual=%.1f dBm\n", powerSet, static_cast<int>(WiFi.getTxPower()) / 4.0);
    } else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED)
      Serial.printf("WiFi disconnected reason=%u\n", info.wifi_sta_disconnected.reason);
    else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP)
      Serial.printf("WiFi connected IP=%s\n", WiFi.localIP().toString().c_str());
  });
  pinMode(BOOT_PIN, INPUT_PULLUP);
  pinMode(BATTERY_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(BATTERY_PIN, ADC_11db);
  sensor.begin(HX_DT, HX_SCK);
  pinMode(HX_DT, INPUT_PULLUP); // An unplugged DT should read not-ready, not float.
  channelChangedAt = millis();
  WiFi.mode(WIFI_STA);
  loadConfiguration();
  WiFi.persistent(false); WiFi.setAutoReconnect(false);
  WiFi.disconnect(false, false); WiFi.setSleep(false);
  uint8_t mac[6]; WiFi.macAddress(mac); selfId = nodeId(mac);
  bootId = esp_random(); if (!bootId) bootId = 1;
  election.reset(new Election(selfId, preference, millis(), esp_random() % 1500));
  receiveQueue = xQueueCreate(24, sizeof(Frame)); transmitQueue = xQueueCreate(24, sizeof(Frame));
  if (!receiveQueue || !transmitQueue || esp_now_init() != ESP_OK) {
    Serial.println("Cannot initialize scale network; restarting"); delay(2000); ESP.restart();
  }
  esp_now_register_recv_cb(onReceive); esp_now_register_send_cb(onSent);
  wifiManager.setConfigPortalBlocking(false); wifiManager.setConfigPortalTimeout(0);
  wifiManager.setConnectTimeout(8); wifiManager.setSaveConnectTimeout(8);
  wifiManager.setDisableConfigPortal(false); wifiManager.setWiFiAutoReconnect(false);
  wifiManager.setWiFiAPChannel(1); wifiManager.setShowInfoErase(false); wifiManager.setShowInfoUpdate(true);
  wifiManager.setPreOtaUpdateCallback([]() {
    Serial.println("OTA upload starting; weighing paused");
    resetSamples = true; dualDiagnostics = false;
    haveReading = false; locked = false;
    nodes[0].reading.flags &= ~(Ready | Locked);
    liveSocket.disconnect();
    if (scanRunning) {
      esp_wifi_scan_stop(); WiFi.scanDelete(); scanRunning = false;
    }
    lastScan = millis();
  });
  wifiManager.setAPStaticIPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  // WiFiManager disables an unconnected STA when starting its AP. ESP-NOW uses STA IDs.
  wifiManager.setAPCallback([](WiFiManager *) { WiFi.enableSTA(true); WiFi.setSleep(false); });
  const char *menu[] = {"custom", "wifi", "info"}; wifiManager.setMenu(menu, 3);
  wifiManager.setCustomMenuHTML("<p><a href='/'><button>Open race scales</button></a></p>"
                                "<p><a href='/settings'><button>Scale settings</button></a></p>"
                                "<p><a href='/diagnostics'><button>HX711 diagnostics</button></a></p>"
                                "<p><a href='/update'><button>Firmware update</button></a></p>");
  wifiManager.setWebServerCallback(registerRoutes);
  wifiManager.setSaveConfigCallback(saveHomeCredentials);
  wifiManager.startWebPortal();
  liveSocket.begin();
  liveSocket.onEvent(onLiveSocket);
  liveSocket.enableHeartbeat(10000, 3000, 2);
  Serial.printf("Scale %s | firmware %s | kit %s | %s | CPU %u MHz | calibration %.6f | offset %.1f | discovery starting\n",
                idText(selfId).c_str(), SCALE_FIRMWARE_VERSION, kit,
                PreferenceNames[static_cast<unsigned>(preference)], getCpuFrequencyMhz(), calibration, offset);
}

void loop() {
  updateBatteryVoltage();
  updateLocalReading();
  liveSocket.loop();
  processNetwork();
  pushLiveUpdates();
  static uint32_t buttonSince = 0;
  if (digitalRead(BOOT_PIN) == LOW) {
    if (!buttonSince) buttonSince = millis();
    if (millis() - buttonSince >= 3000) {
      prefs.putUChar("mode", static_cast<uint8_t>(Preference::Auto));
      prefs.putString("kit", "SCALE"); prefs.putUChar("corner", 0);
      Serial.println("Role/kit/corner reset; calibration and home Wi-Fi retained");
      ESP.restart();
    }
  } else buttonSince = 0;
  if (restartAt && static_cast<int32_t>(millis() - restartAt) >= 0) ESP.restart();
  delay(1);
}
