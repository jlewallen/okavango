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

public:
    void handleTransmissionIfNecessary();
    void sendStatusTransmission();

private:
    String atlasPacketToMessage(atlas_sensors_packet_t *packet);
    String sonarPacketToMessage(sonar_station_packet_t *packet);
    String weatherStationPacketToMessage(weather_station_packet_t *packet);
    String locationToMessage(gps_fix_t *location);

    void sendSensorTransmission(bool sendAtlas, bool sendWeather, bool sendSonar);
    void sendLocationTransmission();
    bool transmission(String message);
};

#endif
