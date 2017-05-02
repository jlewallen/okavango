#ifndef DATA_BOAT_H
#define DATA_BOAT_H

#include "core.h"
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
    atlas_sensors_packet_t *atlasPacket;
    DataBoatLog log;
    Queue queueA;
    Queue queueB;

public:
    DataBoat(atlas_sensors_packet_t *atlasPacket);
    bool setup();
    bool tick();
    void upload();
    void logDataBoatPacketLocally(data_boat_packet_t *reading);

};

#endif
