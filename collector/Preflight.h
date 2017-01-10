#ifndef PREFLIGHT_H_INCLUDED
#define PREFLIGHT_H_INCLUDED

#include "Configuration.h"
#include "WeatherStation.h"

class Preflight {
private:
    Configuration *configuration;
    WeatherStation *weatherStation;

public:
    Preflight(Configuration *configuration, WeatherStation *weatherStation);
    bool check();

private:
    bool checkCommunications();
    bool checkWeatherStation();
};

#endif
