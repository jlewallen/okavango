#ifndef DIAGNOSTICS_H_INCLUDED
#define DIAGNOSTICS_H_INCLUDED

#include <Arduino.h>
#include "FuelGauge.h"
#include "Platforms.h"
#include "core.h"

class Diagnostics {
public:
    uint32_t batterySleepTime = 0;
    uint16_t numberOfTransmissionFailures = 0;
    uint16_t numberOfTransmissionSkipped = 0;
    uint16_t weatherReadingsReceived = 0;
    uint16_t atlasPacketsReceived = 0;
    uint16_t sonarPacketsReceived = 0;
    bool hasGpsFix = false;

public:
    void recordBatterySleep(uint32_t ms) {
        batterySleepTime += ms;
    }

    void recordTransmissionSkipped() {
        numberOfTransmissionSkipped++;
    }

    void recordTransmissionFailure() {
        numberOfTransmissionFailures++;
    }

    void recordSonarPacket() {
        atlasPacketsReceived++;
    }

    void recordAtlasPacket() {
        sonarPacketsReceived++;
    }

    void recordWeatherReading() {
        weatherReadingsReceived++;
    }

    void updateGpsStatus(bool has) {
        hasGpsFix = has;
    }

};

extern Diagnostics diagnostics;

#endif
