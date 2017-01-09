#ifndef DIAGNOSTICS_H_INCLUDED
#define DIAGNOSTICS_H_INCLUDED

#include <Arduino.h>

class Diagnostics {
public:
    uint32_t batterySleepTime = 0;
    uint32_t numberOfTransmissionFailures = 0;

public:
    void recordBatterySleep(uint32_t ms) {
        batterySleepTime += ms;
    }
    void recordTransmissionFailure() {
        numberOfTransmissionFailures++;
    }
};

extern Diagnostics diagnostics;

#endif
