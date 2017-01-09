#ifndef DIAGNOSTICS_H_INCLUDED
#define DIAGNOSTICS_H_INCLUDED

#include <Arduino.h>

class Diagnostics {
private:
    uint32_t batterySleepTime = 0;

public:
    void recordBatterySleep(uint32_t ms) {
        batterySleepTime += ms;
    }
};

extern Diagnostics diagnostics;

#endif
