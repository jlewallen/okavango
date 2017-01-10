#ifndef COLLECTOR_H_INCLUDED
#define COLLECTOR_H_INCLUDED

#include "WeatherStation.h"
#include "Configuration.h"

class Collector {
private:
    WeatherStation *weatherStation;
    Configuration *configuration;
    bool radioSetup = false;

public:
    Collector(WeatherStation *weatherStation, Configuration *configuration) :
        weatherStation(weatherStation), configuration(configuration) {
    }

public:
    void waitForBattery();
    void tick();
    void loop();
    bool checkWeatherStation();
    void logTransition(const char *name);
    void idlePeriod();
    void checkAirwaves();
};

#endif
