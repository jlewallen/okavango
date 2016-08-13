#ifndef DATA_BOAT_H
#define DATA_BOAT_H

#include "core.h"
#include "AttachedGps.h"

class DataBoat {
private:
    CorePlatform corePlatform;
    AttachedGps gps;

public:
    DataBoat(HardwareSerial *gpsStream, uint8_t pinGpsEnable);
    bool setup();
    bool tick();

private:
    void upload(String &json);
    String readingToJson(data_boat_packet_t *reading);
};

#endif
