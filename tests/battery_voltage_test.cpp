#include <cassert>
#include <cstdint>
#include <cstdio>
#include <atomic>

constexpr int BATTERY_PIN = 3;
enum class BatteryLed : uint8_t { Off, Charging, Low };
std::atomic<BatteryLed> ledBattery{BatteryLed::Off};
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
  assert(reads == 8 && batteryMillivolts == 4860);
  assert(ledBattery.load() == BatteryLed::Charging);
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
  assert(batteryMillivolts == 5380); // Exactly 4.5 V still receives the correction.
  for (auto &sample : samples) sample = 2251;
  now += 1000; updateBatteryVoltage();
  assert(batteryMillivolts == 4502); // Just above 4.5 V bypasses it.
  for (auto &sample : samples) sample = 1380;
  now += 1000; updateBatteryVoltage();
  assert(batteryMillivolts == 3640); // Battery operation: 2.76 V VBUS + 0.88 V.
  assert(ledBattery.load() == BatteryLed::Low);
  for (auto &sample : samples) sample = 1459;
  now += 1000; updateBatteryVoltage();
  assert(batteryMillivolts == 3798 && ledBattery.load() == BatteryLed::Low);
  for (auto &sample : samples) sample = 1460;
  now += 1000; updateBatteryVoltage();
  assert(batteryMillivolts == 3800 && ledBattery.load() == BatteryLed::Off);
  for (auto &sample : samples) sample = 1560;
  now += 1000; updateBatteryVoltage();
  assert(batteryMillivolts == 4000 && ledBattery.load() == BatteryLed::Off);
  for (auto &sample : samples) sample = 1660;
  now += 1000; updateBatteryVoltage();
  assert(batteryMillivolts == 4200 && ledBattery.load() == BatteryLed::Off);
  for (auto &sample : samples) sample = 1810;
  now += 1000; updateBatteryVoltage();
  assert(batteryMillivolts == 4500 && ledBattery.load() == BatteryLed::Off);
  for (auto &sample : samples) sample = 1811;
  now += 1000; updateBatteryVoltage();
  assert(batteryMillivolts == 4502 && ledBattery.load() == BatteryLed::Charging);
  std::puts("PASS: GPIO3 battery/USB correction threshold, averaging, cadence and timer wraparound");
}
