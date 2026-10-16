#pragma once

#include "scale_network.h"

namespace scales {

// Keep discovery identity/preference in an AP vendor IE rather than the visible SSID.
#pragma pack(push, 1)
struct GatewayAdvertisement {
  uint8_t header[6] = {0xdd, sizeof(Message) + 10, 0x18, 0xfe, 0x34, 0x53};
  uint8_t stationMac[6]{};
  Message message{};
};
#pragma pack(pop)
static_assert(sizeof(GatewayAdvertisement) == sizeof(Message) + 12, "Gateway IE layout");

inline bool decodeGatewayAdvertisement(const uint8_t *bytes, size_t length,
                                       GatewayAdvertisement &advertisement) {
  if (!bytes || length != sizeof(advertisement)) return false;
  GatewayAdvertisement expected;
  if (std::memcmp(bytes, expected.header, sizeof(expected.header))) return false;
  std::memcpy(&advertisement, bytes, length);
  bool haveMac = false;
  for (uint8_t byte : advertisement.stationMac) haveMac |= byte != 0;
  return haveMac && !(advertisement.stationMac[0] & 1) &&
         decode(bytes + offsetof(GatewayAdvertisement, message), sizeof(Message), advertisement.message) &&
         advertisement.message.kind == Kind::Beacon && advertisement.message.preference != Preference::ScaleOnly &&
         advertisement.message.beaconMetadataVersion == 2;
}

} // namespace scales
