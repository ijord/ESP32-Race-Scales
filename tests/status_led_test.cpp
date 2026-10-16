#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <vector>
#include "led_clock.h"
using scales::LedClock;
using TimerHandle_t = void *;
constexpr int STATUS_RGB_PIN = 48;
std::atomic<bool> ledGateway{false};
LedClock ledClock;
enum class BatteryLed : uint8_t { Off, Charging, Low };
std::atomic<BatteryLed> ledBattery{BatteryLed::Off};
constexpr int WL_CONNECTED = 3;
struct { int state=0; int status() { return state; } } WiFi;
struct Pulse { int tick, red, green, blue; };
std::vector<Pulse> pulses;
int tick = 0;
int64_t esp_timer_get_time() { return static_cast<int64_t>(tick) * 10000; }
void neopixelWrite(int pin, int red, int green, int blue) {
  assert(pin == 48);
  pulses.push_back({tick, red, green, blue});
}
#include "status_led_function.h"
int main() {
  for (tick=0; tick<500; ++tick) {
    if (tick==100) { ledGateway.store(true); WiFi.state=WL_CONNECTED; ledBattery.store(BatteryLed::Charging); }
    if (tick==200) { WiFi.state=0; ledBattery.store(BatteryLed::Low); }
    if (tick==300) { ledGateway.store(false); WiFi.state=WL_CONNECTED; ledBattery.store(BatteryLed::Off); }
    if (tick==400) { ledGateway.store(true); ledBattery.store(BatteryLed::Low); }
    updateStatusLed(nullptr);
  }
  assert(pulses.size()==16);
  const int expected[5][6] = {
    {0,16,0, 0,0,0},    // Member, no battery indication.
    {0,0,16, 16,16,16}, // Gateway associated to Wi-Fi, charging.
    {16,16,0, 16,0,0},  // Gateway AP only, low battery.
    {0,16,0, 0,0,0},    // Member stays green even with Wi-Fi association.
    {0,0,16, 16,0,0}    // Associated gateway, low battery.
  };
  size_t index=0;
  for (size_t cycle=0;cycle<5;++cycle) {
    bool haveBattery=cycle!=0 && cycle!=3;
    const Pulse &network=pulses[index], &off=pulses[index+1];
    assert(network.tick==int(cycle)*100);
    assert(network.red==expected[cycle][0] && network.green==expected[cycle][1] && network.blue==expected[cycle][2]);
    assert(off.tick==network.tick+(haveBattery?20:10));
    assert(off.red==0 && off.green==0 && off.blue==0);
    if(haveBattery) {
      const Pulse &battery=pulses[index+2], &batteryOff=pulses[index+3];
      assert(battery.tick==network.tick+50 && batteryOff.tick==network.tick+70);
      assert(battery.red==expected[cycle][3] && battery.green==expected[cycle][4] && battery.blue==expected[cycle][5]);
      assert(batteryOff.red==0 && batteryOff.green==0 && batteryOff.blue==0);
    }
    index+=haveBattery?4:2;
  }
  // Battery state changes during a cycle must not create extra pulses mid-cycle.
  ledBattery.store(BatteryLed::Off);
  for(tick=500;tick<700;++tick) {
    if(tick==515)ledBattery.store(BatteryLed::Low);
    updateStatusLed(nullptr);
  }
  assert(pulses.size()==22);
  assert(pulses[16].tick==500 && pulses[17].tick==510);
  assert(pulses[18].tick==600 && pulses[19].tick==620 && pulses[20].tick==650 && pulses[21].tick==670);
  puts("PASS: synchronized-phase LED rendering, 100 ms single flash, 200/300/200/300 ms dual flash and colors");
}
