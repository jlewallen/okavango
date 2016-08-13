#ifndef WEATHER_STATION_H
#define WEATHER_STATION_H

#ifdef ARDUINO_SAMD_FEATHER_M0

#include "Platforms.h"

#define FK_WEATHER_STATION_FIELD_UNIXTIME                    0
#define FK_WEATHER_STATION_FIELD_WIND_DIR                    1
#define FK_WEATHER_STATION_FIELD_WIND_SPEED                  2
#define FK_WEATHER_STATION_FIELD_WIND_GUST                   3
#define FK_WEATHER_STATION_FIELD_WIND_GUST_DIR               4
#define FK_WEATHER_STATION_FIELD_WIND_SPEED_2M               5
#define FK_WEATHER_STATION_FIELD_WIND_DIR_2M                 6
#define FK_WEATHER_STATION_FIELD_WIND_GUST_10M               7
#define FK_WEATHER_STATION_FIELD_WIND_GUST_DIR_10m           8
#define FK_WEATHER_STATION_FIELD_HUMIDITY                    9
#define FK_WEATHER_STATION_FIELD_TEMPERATURE                 10
#define FK_WEATHER_STATION_FIELD_RAIN                        11
#define FK_WEATHER_STATION_FIELD_DAILY_RAIN                  12
#define FK_WEATHER_STATION_FIELD_PRESSURE                    13
#define FK_WEATHER_STATION_FIELD_LIGHT_LEVEL                 14
#define FK_WEATHER_STATION_FIELD_LONGITUDE                   15
#define FK_WEATHER_STATION_FIELD_LATITUDE                    16
#define FK_WEATHER_STATION_FIELD_ALTITUDE                    17
#define FK_WEATHER_STATION_FIELD_SATELLITES                  18
#define FK_WEATHER_STATION_FIELD_DATE                        19
#define FK_WEATHER_STATION_FIELD_TIME                        20

#define FK_WEATHER_STATION_MAX_VALUES                        32
#define FK_WEATHER_STATION_MAX_BUFFER                        20

class WeatherStation {
private:
    uint8_t numberOfValues;
    float values[FK_WEATHER_STATION_MAX_VALUES];
    char buffer[FK_WEATHER_STATION_MAX_BUFFER];
    uint8_t length;

public:
    WeatherStation();

public:
    void setup();
    void hup();
    void off();
    void clear();
    bool tick();
    void ignore();
    void logReadingLocally();
    float *getValues() {
        return values;
    }
    uint8_t getNumberOfValues() {
        return numberOfValues;
    }

};

#endif

#endif