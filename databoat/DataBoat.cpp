#include <ArduinoJson.h>
#include <SD.h>
#include <Adafruit_SleepyDog.h>

#include "DataBoat.h"
#include "WifiConnection.h"
#include "AtlasSensorBoard.h"

class DataBoatConfiguration {
private:
    String ssid = "EXPLORER510";
    String psk = "13454684";
    String server = "intotheokavango.org";
    String path = "/ingest/databoat";

public:
    const char *getSsid() { return ssid.c_str(); }
    const char *getPsk() { return psk.c_str(); }
    const char *getServer() { return server.c_str(); }
    const char *getPath() { return path.c_str(); }

public:
    bool read();
};

bool DataBoatConfiguration::read() {
    if (!SD.exists(FK_SETTINGS_CONFIGURATION_FILENAME)) {
        return false;
    }

    File file = SD.open(FK_SETTINGS_CONFIGURATION_FILENAME, FILE_READ);
    if (!file) {
        return false;
    }

    file.close();

    return true;
}


DataBoat::DataBoat(HardwareSerial *gpsStream, uint8_t pinGpsEnable, atlas_sensors_packet_t *atlasPacket) :
    gps(gpsStream, pinGpsEnable), atlasPacket(atlasPacket),
    queueA("DBQ-A.bin"), queueB("DBQ-B.bin") {
}

bool DataBoat::setup(bool enableGps) {
    if (enableGps) {
        gps.setup();
    }
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

        upload();

        log.close();

        return false;
    }

    return true;
}

String DataBoat::readingToJson(data_boat_packet_t *reading) {
    StaticJsonBuffer<1024> jsonBuffer;

    JsonArray &root = jsonBuffer.createArray();

    JsonObject &data = jsonBuffer.createObject();
    root.add(data);
    data["t_utc"] = reading->time;
    data["conductivity"] = reading->conductivity;
    data["salinity"] = reading->salinity;
    data["ph"] = reading->ph;
    data["dissolved_oxygen"] = reading->dissolved_oxygen;
    data["orp"] = reading->orp;
    data["water temp"] = reading->water_temperature;
    data["humidity"] = reading->humidity;
    data["air_temp"] = reading->temperature;
    data["altitude"] = reading->altitude;
    data["barometric_pressure"] = reading->pressure;
    data["gps_lat"] = reading->latitude;
    data["gps_long"] = reading->longitude;
    data["speed"] = reading->speed;

    String json;
    json.reserve(2048);
    root.prettyPrintTo(json);

    return json;
}

void DataBoat::upload() {
    DataBoatConfiguration configuration;
    configuration.read();

    DEBUG_PRINTLN("Uploading...");

    WifiConnection wifi(configuration.getSsid(), configuration.getPsk(), log.stream());
    if (wifi.open()) {
        data_boat_packet_t reading;

        DEBUG_PRINTLN("Connected");

        Queue *writeQueue = NULL;
        Queue *readQueue = NULL;
        if (queueA.size() > 0) {
            DEBUG_PRINT("Uploading A: ");
            DEBUG_PRINTLN(queueA.size());
            writeQueue = &queueB;
            readQueue = &queueA;
        }
        else {
            DEBUG_PRINT("Uploading B: ");
            DEBUG_PRINTLN(queueB.size());
            writeQueue = &queueA;
            readQueue = &queueB;
        }

        if (writeQueue->size() > 0) {
            DEBUG_PRINT("RQ has items?!");
            writeQueue->copyInto(readQueue);
        }


        bool failed = false;

        while (true) {
            uint8_t *packet = readQueue->dequeue();
            if (packet == NULL) {
                break;
            }
            memcpy(packet, (uint8_t *)&reading, sizeof(data_boat_packet_t));
            String json = readingToJson(&reading);

            DEBUG_PRINTLN(json);

            if (failed || !wifi.post(configuration.getServer(), configuration.getPath(), "application/json", json.c_str())) {
                failed = true;
                writeQueue->enqueue((uint8_t *)&reading);
                DEBUG_PRINTLN("Failed");
            }
        }
    }

    wifi.off();
    log.flush();

    DEBUG_PRINTLN("DataBoat Done");
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
