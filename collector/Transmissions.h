#ifndef TRANSMISSIONS_H_INCLUDED
#define TRANSMISSIONS_H_INCLUDED

#include <Arduino.h>
#include "protocol.h"
#include "Core.h"
#include "Configuration.h"
#include "WeatherStation.h"

class Transmissions {
private:
    WeatherStation *weatherStation;
    RtcAbstractSystemClock *systemClock;
    Configuration *configuration;

public:
    Transmissions(WeatherStation *weatherStation, RtcAbstractSystemClock *systemClock, Configuration *configuration);

private:
    String atlasPacketToMessage(atlas_sensors_packet_t *packet);
    String sonarPacketToMessage(sonar_station_packet_t *packet);
    String weatherStationPacketToMessage(weather_station_packet_t *packet);
    bool singleTransmission(String message);
    void handleSensorTransmission(bool triggered, bool sendAtlas, bool sendWeather, bool sendSonar);
    String locationToMessage(gps_fix_t *location);
    void handleLocationTransmission();

public:
    void handleTransmissionIfNecessary();
};


#endif
