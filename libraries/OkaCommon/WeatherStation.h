#ifndef WEATHER_STATION_H
#define WEATHER_STATION_H

#ifdef ARDUINO_SAMD_FEATHER_M0

#include "Platforms.h"

#define WEATHER_STATION_MAX_VALUES 32
#define WEATHER_STATION_MAX_BUFFER 20

class WeatherStation {
public:
    uint8_t numberOfValues;
    float values[WEATHER_STATION_MAX_VALUES];
    char buffer[WEATHER_STATION_MAX_BUFFER];
    uint8_t length;

    WeatherStation();

public:
    void setup();
    void hup();
    void off();
    void clear();
    bool tick();
    void ignore();
    float *getValues() { return values; }
    uint8_t getNumberOfValues() { return numberOfValues; }
    void logReadingLocally();

};

#endif

#endif
