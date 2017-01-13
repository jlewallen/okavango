#ifndef DIAGNOSTICS_H_INCLUDED
#define DIAGNOSTICS_H_INCLUDED

#include <Arduino.h>
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
    String message(Configuration *configuration) {
        uint32_t uptime = millis() / (1000 * 60);
        String message(SystemClock->now());
        message += ",";
        message += configuration->getName();
        message += "," + String(platformBatteryVoltage(), 2);
        message += "," + String(platformBatteryLevel(), 2);
        message += "," + hasGpsFix;
        message += "," + batterySleepTime;
        message += "," + numberOfTransmissionFailures;
        message += "," + numberOfTransmissionSkipped;
        message += "," + weatherReadingsReceived;
        message += "," + atlasPacketsReceived;
        message += "," + sonarPacketsReceived;
        message += ",";
        message += uptime;
        return message;
    }
};

extern Diagnostics diagnostics;

#endif
