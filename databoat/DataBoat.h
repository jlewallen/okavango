#ifndef DATA_BOAT_H
#define DATA_BOAT_H

#include "core.h"
#include "AttachedGps.h"
#include "Queue.h"

class DataBoatLog {
private:
    File file;

public:
    DataBoatLog() {
    }

    void open() {
        file = SD.open("DB.log", FILE_WRITE);
        if (!file) {
            DEBUG_PRINTLN("No log file!");
        }
    }

    Stream &stream() {
        return file;
    }

    void flush() {
        file.flush();
    }

    void close() {
        file.close();
    }
};

class DataBoat {
private:
    CorePlatform corePlatform;
    AttachedGps gps;
    atlas_sensors_packet_t *atlasPacket;
    DataBoatLog log;
    Queue queueA;
    Queue queueB;

public:
    DataBoat(HardwareSerial *gpsStream, uint8_t pinGpsEnable, atlas_sensors_packet_t *atlasPacket);
    bool setup();
    bool tick();

private:
    void upload();
    String readingToJson(data_boat_packet_t *reading);
    void logDataBoatPacketLocally(data_boat_packet_t *reading);
};

#endif
