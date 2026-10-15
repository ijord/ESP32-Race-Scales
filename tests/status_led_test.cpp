#include <atomic>
#include <cassert>
#include <cstdio>
#include <vector>
using TimerHandle_t = void *;
constexpr int STATUS_RGB_PIN = 48;
std::atomic<bool> ledGateway{false};
struct Pulse { int tick, red, green, blue; };
std::vector<Pulse> pulses;
int tick = 0;
void neopixelWrite(int pin, int red, int green, int blue) {
  assert(pin == 48);
  pulses.push_back({tick, red, green, blue});
}
#include "status_led_function.h"
int main() {
  for (tick=0; tick<30; ++tick) {
    if (tick==10) ledGateway.store(true);
    if (tick==20) ledGateway.store(false);
    updateStatusLed(nullptr);
  }
  assert(pulses.size()==6);
  for (size_t i=0; i<pulses.size(); i+=2) {
    assert(pulses[i].tick==int(i/2)*10);
    assert(pulses[i+1].tick==pulses[i].tick+1); // One 100 ms tick on, start every 1000 ms.
    assert(pulses[i].red==0);
    assert(pulses[i+1].red==0 && pulses[i+1].green==0 && pulses[i+1].blue==0);
  }
  assert(pulses[0].green==16 && pulses[0].blue==0);
  assert(pulses[2].green==0 && pulses[2].blue==16);
  assert(pulses[4].green==16 && pulses[4].blue==0);
  puts("PASS: RGB pulse timing and host/member colors");
}
