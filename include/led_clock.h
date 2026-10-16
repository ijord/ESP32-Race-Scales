#pragma once
#include <atomic>
#include <cstdint>

namespace scales {
class LedClock {
 public:
  uint16_t phase(uint64_t milliseconds) const {
    return (milliseconds % 1000 + offset_.load()) % 1000;
  }
  void synchronize(uint32_t gatewayPhase, uint16_t receivePhase) {
    if (gatewayPhase < 1000 && receivePhase < 1000)
      offset_.store((gatewayPhase + 1000 - receivePhase) % 1000);
  }
  void reset() { offset_.store(0); }
 private:
  std::atomic<uint32_t> offset_{0};
};
} // namespace scales
