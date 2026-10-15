#include "scale_network.h"
#include "firmware_version.h"
#include <cassert>
#include <cstdio>
#include <limits>

using namespace scales;

void electionTests() {
  Election alone(10, Preference::Auto, 0);
  alone.tick(DiscoveryMs - 1);
  assert(alone.role() == Role::Searching);
  alone.tick(DiscoveryMs);
  assert(alone.role() == Role::Gateway && alone.gateway() == 10);

  Candidate host{20, Preference::Auto};
  Election late(5, Preference::PreferGateway, 0);
  late.scanned(&host, 1, 100);
  late.tick(DiscoveryMs);
  assert(late.role() == Role::Member && late.gateway() == 20);
  Candidate newcomer{1, Preference::PreferGateway};
  late.scanned(&newcomer, 1, 200);
  late.heartbeat(99, 10000); // Other kits/devices cannot keep a dead host alive.
  late.tick(100 + GatewayTimeoutMs - 1);
  assert(late.role() == Role::Member);
  late.tick(100 + GatewayTimeoutMs);
  assert(late.role() == Role::Searching);
  late.tick(100 + GatewayTimeoutMs + DiscoveryMs);
  assert(late.role() == Role::Gateway);

  Election follower(10, Preference::ScaleOnly, 0);
  follower.tick(100000);
  assert(follower.role() == Role::Searching);
  follower.scanned(&host, 1, 100001);
  assert(follower.role() == Role::Member);

  Candidate peers[] = {{30, Preference::Auto}, {40, Preference::PreferGateway},
                       {20, Preference::PreferGateway}, {1, Preference::ScaleOnly}};
  Election selector(100, Preference::Auto, 0);
  selector.scanned(peers, 4, 1);
  assert(selector.gateway() == 20);

  // Simultaneous gateways and healed partitions converge to the same winner.
  Election a(10, Preference::Auto, 0), b(20, Preference::Auto, 0);
  a.tick(DiscoveryMs); b.tick(DiscoveryMs);
  Candidate ca{10, Preference::Auto}, cb{20, Preference::Auto};
  a.scanned(&cb, 1, 7000); b.scanned(&ca, 1, 7000);
  assert(a.role() == Role::Gateway && b.role() == Role::Member);
  Candidate preferred{30, Preference::PreferGateway};
  a.scanned(&preferred, 1, 8000);
  assert(a.role() == Role::Member && a.gateway() == 30);

  Election wrap(10, Preference::Auto, UINT32_MAX - 1000);
  wrap.tick(4998);
  assert(wrap.role() == Role::Searching);
  wrap.tick(4999);
  assert(wrap.role() == Role::Gateway);
  Election wrapMember(10, Preference::Auto, UINT32_MAX - 1000);
  wrapMember.scanned(&host, 1, UINT32_MAX - 500);
  wrapMember.heartbeat(20, 2000);
  wrapMember.tick(21999);
  assert(wrapMember.role() == Role::Member);
  wrapMember.tick(22000);
  assert(wrapMember.role() == Role::Searching);
}

void protocolTests() {
  Message m{};
  m.magic = Magic; m.version = Version; m.kind = Kind::Reading;
  m.channel = 6; m.boot = 1; m.sequence = 1;
  std::strcpy(m.kit, "RACE");
  Message decoded;
  auto valid = [&]() { return decode(reinterpret_cast<uint8_t *>(&m), sizeof(m), decoded); };
  assert(valid());
  // Legacy packets have an empty version. New readings preserve the old layout
  // and carry signed weights plus a version in the unused configuration field.
  static_assert(sizeof(Message) == 64, "Legacy wire size");
  static_assert(offsetof(Message, firmwareVersion) == offsetof(Message, configKit), "Version wire offset");
  m.weight = -12.5f;
  std::strcpy(m.firmwareVersion, SCALE_FIRMWARE_VERSION);
  assert(valid() && decoded.weight == -12.5f);
  assert(std::strcmp(decoded.firmwareVersion, SCALE_FIRMWARE_VERSION) == 0);
  static_assert(offsetof(Message, calibration) == offsetof(Message, knownWeight), "Calibration wire offset");
  static_assert(offsetof(Message, zeroOffset) == offsetof(Message, battery), "Zero-offset wire offset");
  m.readingMetadataVersion = 1;
  m.calibration = -2978.173096f; m.zeroOffset = -178485.2f;
  assert(valid() && decoded.readingMetadataVersion == 1);
  assert(decoded.calibration == m.calibration && decoded.zeroOffset == m.zeroOffset);
  static_assert(offsetof(Message, batteryMillivolts) == offsetof(Message, requesterBoot), "Battery wire offset");
  m.readingMetadataVersion = 2; m.batteryMillivolts = 4180;
  assert(valid() && decoded.batteryMillivolts == 4180 && decoded.readingMetadataVersion == 2);
  assert(decoded.calibration == m.calibration && decoded.zeroOffset == m.zeroOffset);
  static_assert(offsetof(Message, voltageMillivolts) == offsetof(Message, requesterBoot), "Dual-voltage wire offset");
  m.readingMetadataVersion = 3;
  m.voltageMillivolts.pin1 = 4180; m.voltageMillivolts.pin3 = 3980;
  assert(valid() && decoded.readingMetadataVersion == 3);
  assert(decoded.voltageMillivolts.pin1 == 4180 && decoded.voltageMillivolts.pin3 == 3980);
  assert(decoded.calibration == m.calibration && decoded.zeroOffset == m.zeroOffset);
  m.readingMetadataVersion = 4; m.batteryMillivolts = 4950;
  assert(valid() && decoded.readingMetadataVersion == 4 && decoded.batteryMillivolts == 4950);
  assert(decoded.calibration == m.calibration && decoded.zeroOffset == m.zeroOffset);
  m.calibration = std::numeric_limits<float>::infinity(); assert(!valid());
  m.calibration = -2978.173096f;
  m.zeroOffset = std::numeric_limits<float>::quiet_NaN(); assert(!valid());
  m.zeroOffset = -178485.2f;
  std::memset(m.firmwareVersion, '1', sizeof(m.firmwareVersion));
  assert(!valid()); // No terminator.
  std::strcpy(m.firmwareVersion, "bad\"name");
  assert(!valid());
  std::memset(m.firmwareVersion, 0, sizeof(m.firmwareVersion));
  m.readingMetadataVersion = 0; m.battery = m.knownWeight = 0; // Legacy reading.
  assert(valid());
  assert(!decode(reinterpret_cast<uint8_t *>(&m), sizeof(m) - 1, decoded));
  m.version++; assert(!valid()); m.version = Version;
  m.magic = 0; assert(!valid()); m.magic = Magic;
  std::memset(m.kit, 'A', sizeof(m.kit)); assert(!valid()); std::strcpy(m.kit, "RACE");
  m.weight = std::numeric_limits<float>::quiet_NaN(); assert(!valid()); m.weight = 0;
  m.raw = std::numeric_limits<float>::infinity(); assert(!valid()); m.raw = 0;
  m.channel = 0; assert(!valid()); m.channel = 6;
  m.corner = static_cast<Corner>(5); assert(!valid()); m.corner = Corner::LF;
  m.flags = 8; assert(!valid()); m.flags = Ready | Calibrated;
  m.kind = Kind::Command; m.command = Command::Calibrate;
  m.knownWeight = -1; assert(!valid()); m.knownWeight = 50; assert(valid());
  m.command = Command::Configure; assert(!valid());
  std::strcpy(m.configKit, "KIT2"); assert(valid());
  m.configPreference = static_cast<Preference>(3); assert(!valid());
  m.configPreference = Preference::Auto;
  m.kind = Kind::Ack; m.requesterBoot = 0; assert(!valid()); m.requesterBoot = 1; assert(valid());
  m.result = static_cast<Result>(6); assert(!valid());
  assert(validKit("RACE") && validKit("12345678"));
  assert(!validKit("") && !validKit("lower") && !validKit("AB-CD"));
}

void commandTests() {
  CommandHistory history;
  history.remember(10, 1, UINT32_MAX, Result::Ok);
  assert(history.duplicate(10, 1, UINT32_MAX));
  assert(history.result() == Result::Ok);
  assert(history.stale(10, 1, UINT32_MAX - 1));
  assert(!history.stale(10, 1, 1)); // Sequence wrap is valid.
  assert(!history.duplicate(10, 2, UINT32_MAX)); // A new gateway boot has a new session.
  assert(!history.stale(20, 1, 1));
}

int main() {
  electionTests(); protocolTests(); commandTests();
  std::puts("PASS: election, failover, wraparound, protocol validation, command retries");
}
