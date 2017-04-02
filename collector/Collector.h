#ifndef COLLECTOR_H_INCLUDED
#define COLLECTOR_H_INCLUDED

#include "WeatherStation.h"
#include "Configuration.h"
#include "TransmissionStatus.h"
#include "FuelGauge.h"
#include "LoraRadio.h"
#include "Memory.h"
#include "core.h"

enum class CollectorState {
    Airwaves,
    Idle,
    Transmission
};

class Collector {
private:
    CollectorState state = CollectorState::Airwaves;
    TransmissionStatus status;
    FuelGauge gauge;
    CorePlatform corePlatform;
    Configuration configuration;
    WeatherStation weatherStation;
    LoraRadio radio;
    Memory memory;
    bool sendStatus = false;

public:
    Collector();

public:
    void setup();
    void loop();

private:
    uint32_t deepSleep(uint32_t ms);

private:
    void preflight();
    void waitForBattery();
    void tick();
    void logTransition(const char *name);
    void idlePeriod();
    void checkAirwaves();
    bool sendStatusTransmission();
    bool quickTransmissionCheck();

};

#endif
