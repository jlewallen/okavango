// Be careful to use a platform-specific conditional include to only make the
// code visible for the appropriate platform.  Arduino will try to compile and
// link all .cpp files regardless of platform.
#if defined(ARDUINO_ARCH_SAMD)

#include <sam.h>
#include <wdt.h>
#include <system.h>
#include "WatchdogSAMD.h"

int WatchdogSAMD::enable(int maxPeriodMs) {
    int actualMs;
    uint8_t period;
    if (maxPeriodMs >= 8192) {
      actualMs = 8192;
      period = 0xA;
    }
    else if (maxPeriodMs >= 4096) {
      actualMs = 4096;
      period = 0x9;
    }
    else if (maxPeriodMs >= 2048) {
      actualMs = 2048;
      period = 0x8;
    }
    else if (maxPeriodMs >= 1024) {
      actualMs = 1024;
      period = 0x7;
    }
    else if (maxPeriodMs >= 512) {
      actualMs = 512;
      period = 0x6;
    }
    else if (maxPeriodMs >= 256) {
      actualMs = 256;
      period = 0x5;
    }
    else if (maxPeriodMs >= 128) {
      actualMs = 128;
      period = 0x4;
    }
    else if (maxPeriodMs >= 64) {
      actualMs = 64;
      period = 0x3;
    }
    else if (maxPeriodMs >= 32) {
      actualMs = 32;
      period = 0x2;
    }
    else if (maxPeriodMs >= 16) {
      actualMs = 16;
      period = 0x1;
    }
    else {
      return 0;
    }

    if (wdt_enable(period)) {
    }

    return actualMs;
}

void WatchdogSAMD::reset() {
    wdt_checkin();
}

void WatchdogSAMD::disable() {
    wdt_disable();
}

int WatchdogSAMD::sleep(int ms) {
    wdt_disable();

    while (ms > 0) {
        int sleepingFor = wdt_enable(ms);
        ms -= sleepingFor;
        if (sleepingFor > 0) {
            Serial.println("Sleeping...");

            delay(100);
            system_deep_sleep();

            Serial.println("Awake...");
        }
        else {
            delay(ms);
            break;
        }
    }

    wdt_enable(ms);
    return 0;
}

#endif
