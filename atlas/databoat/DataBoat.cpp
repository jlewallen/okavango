#include <ArduinoJson.h>
#include <SD.h>
#include <Adafruit_SleepyDog.h>

#include "DataBoat.h"
#include "WifiConnection.h"
#include "AtlasSensorBoard.h"

DataBoat::DataBoat(HardwareSerial *gpsStream, atlas_sensors_packet_t *atlasPacket) :
    gps(gpsStream, atlasPacket(atlasPacket),
    queueA("DBQ-A.bin"), queueB("DBQ-B.bin") {
}

bool DataBoat::setup() {
    gps.setup();
    log.open();

    return true;
}

bool DataBoat::tick() {
    data_boat_packet_t reading;
    memzero((uint8_t *)&reading, sizeof(data_boat_packet_t));

    reading.water_temperature = atlasPacket->values[FK_ATLAS_SENSORS_FIELD_WATER_TEMPERATURE];
    reading.conductivity      = atlasPacket->values[FK_ATLAS_SENSORS_FIELD_EC];
    reading.salinity          = atlasPacket->values[FK_ATLAS_SENSORS_FIELD_SALINITY];
    reading.ph                = atlasPacket->values[FK_ATLAS_SENSORS_FIELD_PH];
    reading.dissolved_oxygen  = atlasPacket->values[FK_ATLAS_SENSORS_FIELD_DO];
    reading.orp               = atlasPacket->values[FK_ATLAS_SENSORS_FIELD_ORP];
    reading.temperature       = atlasPacket->values[FK_ATLAS_SENSORS_FIELD_AIR_TEMPERATURE];
    reading.humidity          = atlasPacket->values[FK_ATLAS_SENSORS_FIELD_HUMIDITY];
    reading.pressure          = atlasPacket->values[FK_ATLAS_SENSORS_FIELD_PRESSURE]; // If Bme280

    if (gps.tick(&reading)) {
        Queue *writeQueue = NULL;
        Queue backupQueue(FK_SETTINGS_BACKUP_DATA_FILENAME);

        DEBUG_PRINTLN("Ok");

        if (queueA.size() > 0) {
            writeQueue = &queueA;
            DEBUG_PRINT("Enqueue A: "); DEBUG_PRINTLN(queueA.size());
            DEBUG_PRINT("Ignore B: "); DEBUG_PRINTLN(queueB.size());
        }
        else {
            writeQueue = &queueB;
            DEBUG_PRINT("Enqueue B: "); DEBUG_PRINTLN(queueB.size());
            DEBUG_PRINT("Ignore A: "); DEBUG_PRINTLN(queueA.size());
        }
        writeQueue->enqueue((uint8_t *)&reading);
        backupQueue.enqueue((uint8_t *)&reading);
        logDataBoatPacketLocally(&reading);

        delay(1000);

        log.close();

        return false;
    }

    return true;
}

void DataBoat::logDataBoatPacketLocally(data_boat_packet_t *reading) {
    File file = Logger::open(FK_SETTINGS_DATA_BOAT_DATA_FILENAME);
    if (file) {
        file.print(reading->time);
        file.print(",");
        file.print(reading->conductivity);
        file.print(",");
        file.print(reading->salinity);
        file.print(",");
        file.print(reading->ph);
        file.print(",");
        file.print(reading->dissolved_oxygen);
        file.print(",");
        file.print(reading->orp);
        file.print(",");
        file.print(reading->water_temperature);
        file.print(",");
        file.print(reading->humidity);
        file.print(",");
        file.print(reading->temperature);
        file.print(",");
        file.print(reading->altitude);
        file.print(",");
        file.print(reading->pressure);
        file.print(",");
        file.print(reading->latitude, 6);
        file.print(",");
        file.print(reading->longitude, 6);
        file.print(",");
        file.print(reading->speed);

        file.println();
        file.close();
    }
}
