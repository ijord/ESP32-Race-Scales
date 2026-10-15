#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace scales {

enum class Preference : uint8_t { Auto, PreferGateway, ScaleOnly };
enum class Role : uint8_t { Searching, Gateway, Member };
// Keep persisted/wire IDs unchanged when renaming the printed corner labels.
enum class Corner : uint8_t { Unassigned = 0, LF = 1, RF = 2, LR = 3, RR = 4 };
enum class Kind : uint8_t { Beacon = 1, Reading, Command, Ack };
enum class Command : uint8_t { Tare = 1, Calibrate, Configure };
enum class Result : uint8_t { Ok, Offline, Invalid, Stale, Timeout };

constexpr uint32_t Magic = 0x53434c32;
constexpr uint8_t Version = 1;
constexpr uint8_t Ready = 1, Calibrated = 2, Locked = 4;
constexpr uint32_t GatewayTimeoutMs = 20000;
constexpr uint32_t DiscoveryMs = 6000;

inline bool validKit(const char *kit) {
  if (!kit[0]) return false;
  for (size_t i = 0; i < 9; ++i) {
    if (!kit[i]) return true;
    if (!((kit[i] >= 'A' && kit[i] <= 'Z') ||
          (kit[i] >= '0' && kit[i] <= '9'))) return false;
  }
  return false;
}

inline bool newer(uint32_t value, uint32_t previous) {
  return static_cast<int32_t>(value - previous) > 0;
}

struct Candidate {
  uint64_t id;
  Preference preference;
  Candidate(uint64_t id = 0, Preference preference = Preference::Auto)
      : id(id), preference(preference) {}
};

inline bool better(const Candidate &a, const Candidate &b) {
  if (a.preference != b.preference)
    return a.preference == Preference::PreferGateway;
  return a.id < b.id;
}

// Platform-independent election policy, shared by the firmware and host tests.
class Election {
 public:
  Election(uint64_t id, Preference preference, uint32_t now, uint32_t jitter = 0)
      : self_{id, preference}, searchStarted_(now), waitMs_(DiscoveryMs + jitter) {}

  Role role() const { return role_; }
  uint64_t gateway() const { return gateway_; }

  void scanned(const Candidate *candidates, size_t count, uint32_t now) {
    if (role_ == Role::Member) return; // An established gateway is not preempted.
    Candidate winner{};
    for (size_t i = 0; i < count; ++i) {
      const Candidate &candidate = candidates[i];
      if (!candidate.id || candidate.id == self_.id ||
          candidate.preference == Preference::ScaleOnly) continue;
      if (!winner.id || better(candidate, winner)) winner = candidate;
    }
    if (!winner.id) return;
    if (role_ == Role::Gateway && !better(winner, self_)) return;
    gateway_ = winner.id;
    lastHeartbeat_ = now;
    role_ = Role::Member;
  }

  void heartbeat(uint64_t id, uint32_t now) {
    if (role_ == Role::Member && id == gateway_) lastHeartbeat_ = now;
  }

  void tick(uint32_t now) {
    if (role_ == Role::Member && now - lastHeartbeat_ >= GatewayTimeoutMs) {
      role_ = Role::Searching;
      gateway_ = 0;
      searchStarted_ = now;
    }
    if (role_ == Role::Searching && self_.preference != Preference::ScaleOnly &&
        now - searchStarted_ >= waitMs_) {
      role_ = Role::Gateway;
      gateway_ = self_.id;
    }
  }

 private:
  Candidate self_;
  Role role_ = Role::Searching;
  uint64_t gateway_ = 0;
  uint32_t searchStarted_, waitMs_, lastHeartbeat_ = 0;
};

#pragma pack(push, 1)
struct Message {
  uint32_t magic;
  uint8_t version;
  Kind kind;
  uint8_t flags;
  Corner corner;
  Preference preference;
  uint8_t channel;
  char kit[9];
  uint32_t boot;
  uint32_t sequence;
  Command command;
  Result result;
  union {
    uint32_t requesterBoot; // ACKs.
    uint32_t batteryMillivolts; // v2: GPIO1 x2; v4: GPIO3 x2 + 970 mV.
    struct {
      uint16_t pin1, pin3; // Reading metadata v3, both multiplied by two.
    } voltageMillivolts;
  };
  float raw, weight;
  union {
    float battery;
    float zeroOffset; // Reading metadata v1-v4.
  };
  union {
    float knownWeight; // Calibrate commands.
    float calibration; // Reading metadata v1-v4, counts per pound.
  };
  uint32_t ip;
  union {
    char configKit[9];       // Configure commands.
    char firmwareVersion[9]; // Beacons/readings; empty in older firmware.
  };
  union {
    Corner configCorner;
    uint8_t readingMetadataVersion; // 1 = calibration; 2/3 = legacy voltage; 4 = corrected GPIO3 battery.
  };
  Preference configPreference;
};
#pragma pack(pop)
static_assert(sizeof(Message) <= 250, "Message must fit ESP-NOW v1");
static_assert(sizeof(Message) == 64, "Keep compatibility with existing scale firmware");

inline bool decode(const uint8_t *bytes, size_t length, Message &message) {
  if (length != sizeof(Message)) return false;
  std::memcpy(&message, bytes, sizeof(message));
  if (message.magic != Magic || message.version != Version ||
      !validKit(message.kit) || !message.boot || !message.sequence ||
      message.kind < Kind::Beacon || message.kind > Kind::Ack ||
      message.corner > Corner::RR || message.preference > Preference::ScaleOnly ||
      message.channel < 1 || message.channel > 14 || (message.flags & ~7) ||
      !std::isfinite(message.raw) || !std::isfinite(message.weight) ||
      !std::isfinite(message.battery) || !std::isfinite(message.knownWeight))
    return false;
  if (message.kind == Kind::Beacon || message.kind == Kind::Reading) {
    // Keep the version safe to include in JSON while accepting legacy empty versions.
    bool terminated = false;
    for (char c : message.firmwareVersion) {
      if (!c) { terminated = true; break; }
      if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') || c == '.' || c == '-')) return false;
    }
    if (!terminated) return false;
  }
  if (message.kind == Kind::Command) {
    if (message.command < Command::Tare || message.command > Command::Configure)
      return false;
    if (message.command == Command::Calibrate && message.knownWeight <= 0)
      return false;
    if (message.command == Command::Configure &&
        (!validKit(message.configKit) || message.configCorner > Corner::RR ||
         message.configPreference > Preference::ScaleOnly)) return false;
  }
  if (message.kind == Kind::Ack &&
      (!message.requesterBoot || message.result > Result::Timeout)) return false;
  return true;
}

// Cache the last result so a lost ACK cannot cause a repeated tare/calibration.
class CommandHistory {
 public:
  bool duplicate(uint64_t sender, uint32_t boot, uint32_t sequence) const {
    return sender == sender_ && boot == boot_ && sequence == sequence_;
  }
  bool stale(uint64_t sender, uint32_t boot, uint32_t sequence) const {
    return sender == sender_ && boot == boot_ && !newer(sequence, sequence_);
  }
  void remember(uint64_t sender, uint32_t boot, uint32_t sequence, Result result) {
    sender_ = sender; boot_ = boot; sequence_ = sequence; result_ = result;
  }
  Result result() const { return result_; }
 private:
  uint64_t sender_ = 0;
  uint32_t boot_ = 0, sequence_ = 0;
  Result result_ = Result::Invalid;
};

} // namespace scales
