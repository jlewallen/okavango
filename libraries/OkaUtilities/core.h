#ifndef CORE_H
#define CORE_H

#include <RTClib.h>
#include "Platforms.h"

class CorePlatform {
public:
    void setup();

};

class RtcSystemClock {
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
