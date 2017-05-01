#ifndef ATTACHED_GPS_H
#define ATTACHED_GPS_H

#include <Adafruit_GPS.h>
#include <RTClib.h>
#include "protocol.h"

class AttachedGps {
private:
    uint8_t pinGpsEnable;
    Adafruit_GPS gps;

public:
    AttachedGps(HardwareSerial *serial, uint8_t pinGpsEnable);
    void setup();
    bool tick(data_boat_packet_t *packet);

};

#endif
