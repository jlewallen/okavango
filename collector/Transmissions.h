#ifndef TRANSMISSIONS_H_INCLUDED
#define TRANSMISSIONS_H_INCLUDED

#include <Arduino.h>
#include "protocol.h"
#include "core.h"
#include "Configuration.h"
#include "WeatherStation.h"
#include "TransmissionStatus.h"

class Transmissions {
private:
    WeatherStation *weatherStation;
    RtcAbstractSystemClock *systemClock;
    Configuration *configuration;
    TransmissionStatus *status;

public:
    Transmissions(WeatherStation *weatherStation, RtcAbstractSystemClock *systemClock, Configuration *configuration, TransmissionStatus *status);

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
