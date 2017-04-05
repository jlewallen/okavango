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
    uint32_t deadFor = 0;
    float totalTransmissionTime = 0.0;
    uint16_t numberOfTransmissions = 0;
    bool hasGpsFix = false;
    bool communicationsPassed = false;
    bool weatherStationPassed = false;
    bool loraPassed = false;

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

    void recordDeadFor(uint32_t time) {
        deadFor = time;
    }

    void recordPreflight(bool communications, bool weatherStation, bool lora) {
        communicationsPassed = communications;
        weatherStationPassed = weatherStation;
        loraPassed = lora;
    }

    void recordTransmission(uint32_t time) {
        totalTransmissionTime += ((float)time) / 1000.0f;
        numberOfTransmissions++;
    }

    float getAverageTransmissionTime() {
        if (numberOfTransmissions == 0) {
            return 0.0f;
        }
        return totalTransmissionTime / (float)numberOfTransmissions;
    }

    uint32_t getUptime() {
        return platformUptime() / (1000 * 60);
    }

};

extern Diagnostics diagnostics;

#endif
