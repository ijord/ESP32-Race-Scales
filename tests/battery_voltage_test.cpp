#include <cassert>
#include <cstdint>
#include <cstdio>

constexpr int BATTERY_PIN = 3;
uint32_t batteryMillivolts = 0, now = 0, reads = 0;
uint32_t samples[8] = {1990, 2000, 1980, 1990, 2000, 1980, 1990, 1990};
uint32_t millis() { return now; }
uint32_t analogReadMilliVolts(int pin) {
  assert(pin == 3);
  return samples[reads++ % 8];
}
#include "battery_voltage_function.h"

int main() {
  updateBatteryVoltage();
  assert(reads == 8 && batteryMillivolts == 4950);
  now = 999; updateBatteryVoltage();
  assert(reads == 8);
  for (auto &sample : samples) sample = 2500;
  now = 1000; updateBatteryVoltage();
  assert(reads == 16 && batteryMillivolts == 5000); // USB VBUS: no correction.
  now = UINT32_MAX - 500; updateBatteryVoltage();
  assert(reads == 24);
  now = 498; updateBatteryVoltage();
  assert(reads == 24);
  now = 499; updateBatteryVoltage();
  assert(reads == 32 && batteryMillivolts == 5000);
  for (auto &sample : samples) sample = 2250;
  now += 1000; updateBatteryVoltage();
  assert(batteryMillivolts == 5470); // Exactly 4.5 V still receives the correction.
  for (auto &sample : samples) sample = 2251;
  now += 1000; updateBatteryVoltage();
  assert(batteryMillivolts == 4502); // Just above 4.5 V bypasses it.
  for (auto &sample : samples) sample = 1380;
  now += 1000; updateBatteryVoltage();
  assert(batteryMillivolts == 3730); // Battery operation: 2.76 V VBUS + 0.97 V.
  std::puts("PASS: GPIO3 battery/USB correction threshold, averaging, cadence and timer wraparound");
}
