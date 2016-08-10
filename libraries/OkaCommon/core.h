#ifndef CORE_H
#define CORE_H

#include <RTClib.h>
#include "Platforms.h"

class CorePlatform {
private:
    RTC_PCF8523 rtc;

public:
    void setup();
    uint32_t now();

};

#endif
