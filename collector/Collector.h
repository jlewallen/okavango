#ifndef COLLECTOR_H_INCLUDED
#define COLLECTOR_H_INCLUDED

#include "WeatherStation.h"
#include "Configuration.h"
#include "TransmissionStatus.h"
#include "FuelGauge.h"

enum class CollectorState {
    Airwaves,
    WeatherStation,
    Idle,
    Transmission
};

class Collector {
private:
    WeatherStation *weatherStation;
    Configuration *configuration;
    bool radioSetup = false;
    CollectorState state = CollectorState::Airwaves;
    TransmissionStatus status;
    FuelGauge gauge;

public:
    Collector(WeatherStation *weatherStation, Configuration *configuration) :
        weatherStation(weatherStation), configuration(configuration) {
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
