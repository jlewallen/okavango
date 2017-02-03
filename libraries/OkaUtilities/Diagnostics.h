#ifndef DIAGNOSTICS_H_INCLUDED
#define DIAGNOSTICS_H_INCLUDED

#include <Arduino.h>
#include "FuelGauge.h"
#include "Configuration.h"
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
    String message(FuelGauge *fuelGauge, Configuration *configuration) {
        uint32_t uptime = millis() / (1000 * 60);
        String message(SystemClock->now());
        message += ",";
        message += configuration->getName();
        message += "," + String(fuelGauge->cellVoltage(), 2);
        message += "," + String(fuelGauge->stateOfCharge(), 2);
        message += "," + String(hasGpsFix);
        message += "," + String(batterySleepTime);
        message += "," + String(numberOfTransmissionFailures);
        message += "," + String(numberOfTransmissionSkipped);
        message += "," + String(weatherReadingsReceived);
        message += "," + String(atlasPacketsReceived);
        message += "," + String(sonarPacketsReceived);
        message += ",";
        message += uptime;
        return message;
    }
};

extern Diagnostics diagnostics;

#endif
