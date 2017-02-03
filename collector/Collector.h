#ifndef COLLECTOR_H_INCLUDED
#define COLLECTOR_H_INCLUDED

#include "WeatherStation.h"
#include "Configuration.h"
#include "TransmissionStatus.h"
#include "FuelGauge.h"
#include "core.h"

enum class CollectorState {
    Airwaves,
    WeatherStation,
    Idle,
    Transmission
};

class Collector {
private:
    bool radioSetup = false;
    CollectorState state = CollectorState::Airwaves;
    TransmissionStatus status;
    FuelGauge gauge;
    CorePlatform corePlatform;
    Configuration configuration;
    WeatherStation weatherStation;

public:
    Collector() : configuration(FK_SETTINGS_CONFIGURATION_FILENAME) {
    }

public:
    void setup();
    void waitForBattery();
    void tick();
    void loop();
    bool checkWeatherStation();
    void logTransition(const char *name);
    void idlePeriod();
    void checkAirwaves();

};

#endif
