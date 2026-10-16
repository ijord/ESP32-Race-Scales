#include "led_clock.h"
#include <cassert>
#include <cstdio>
using scales::LedClock;

int main() {
  LedClock gateway,memberA,memberB;
  const uint64_t gatewayStart=100000,memberAStart=56789,memberBStart=12345;
  const uint64_t transmitted=123;
  uint16_t beaconPhase=gateway.phase(gatewayStart+transmitted);
  memberA.synchronize(beaconPhase,(memberAStart+transmitted)%1000);
  memberB.synchronize(beaconPhase,(memberBStart+transmitted)%1000);
  // Even if the receive queue is processed late, use its captured arrival phase.
  for(uint64_t elapsed=900;elapsed<10000;elapsed+=7) {
    assert(memberA.phase(memberAStart+elapsed)==gateway.phase(gatewayStart+elapsed));
    assert(memberB.phase(memberBStart+elapsed)==gateway.phase(gatewayStart+elapsed));
  }
  // Keep running with the last clock offset while gateway beacons are missing.
  assert(memberA.phase(memberAStart+500000)==gateway.phase(gatewayStart+500000));
  uint16_t before=memberA.phase(memberAStart);
  memberA.synchronize(1000,0);memberA.synchronize(0,1000);
  assert(memberA.phase(memberAStart)==before);
  // Millis rollover cannot reset the cycle: the firmware uses the 64-bit ESP timer.
  uint64_t rollover=uint64_t(UINT32_MAX)+1;
  assert(gateway.phase(rollover-1)==295);
  assert(gateway.phase(rollover)==296);
  assert(gateway.phase(rollover+1)==297);
  // A new/rebooted gateway advertises its own phase; members re-align to it.
  LedClock replacement;
  memberA.synchronize(replacement.phase(91234),(memberAStart+700)%1000);
  assert(memberA.phase(memberAStart+700)==replacement.phase(91234));
  memberA.reset();assert(memberA.phase(memberAStart)==memberAStart%1000);
  // One-way transport latency remains; a 4 ms delay gives 4 ms phase lag.
  memberB.synchronize(beaconPhase,(memberBStart+transmitted+4)%1000);
  assert((gateway.phase(gatewayStart+5000)+1000-memberB.phase(memberBStart+5000))%1000==4);
  puts("PASS: multi-device LED alignment, delayed processing, holdover, 32-bit rollover and gateway changes");
}
