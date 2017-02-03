#ifndef PREFLIGHT_H_INCLUDED
#define PREFLIGHT_H_INCLUDED

#include "Configuration.h"
#include "WeatherStation.h"
#include "LoraRadio.h"

class Preflight {
private:
    Configuration *configuration;
    WeatherStation *weatherStation;
    LoraRadio *radio;

public:
    Preflight(Configuration *configuration, WeatherStation *weatherStation, LoraRadio *radio);
    bool check();

private:
    bool checkCommunications();
    bool checkWeatherStation();
};

#endif
