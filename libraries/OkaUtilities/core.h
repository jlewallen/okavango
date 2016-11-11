#ifndef CORE_H
#define CORE_H

#include <RTClib.h>
#include "Platforms.h"

class CorePlatform {
public:
    void setup(uint8_t pinSdCs, uint8_t pinRfm95Cs, uint8_t pinRfm95Rst, bool haveClock = true);

};

class RtcSystemClock {
private:
    bool available;
    RTC_PCF8523 rtc;
    uint32_t adjusted = 0;

public:
    bool setup();
    bool initialized();
    uint32_t now();
    bool set(uint32_t now);
};

extern RtcSystemClock SystemClock;

#endif
