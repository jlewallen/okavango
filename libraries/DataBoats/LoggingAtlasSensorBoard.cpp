#include <Adafruit_GPS.h>

#include "LoggingAtlasSensorBoard.h"
#include "Logger.h"
#include "Queue.h"

LoggingAtlasSensorBoard::LoggingAtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *serialPortExpander, SensorBoard *sensorBoard, FuelGauge *fuelGauge, DataBoatReadingHandler *handler) :
    AtlasSensorBoard(corePlatform, serialPortExpander, sensorBoard, fuelGauge), handler(handler) {
}

void LoggingAtlasSensorBoard::done(SensorBoard *board) {
    size_t numberOfValues = 0;

    for (uint8_t i = 0; i < board->getNumberOfValues(); ++i) {
        if (numberOfValues < FK_ATLAS_SENSORS_PACKET_NUMBER_VALUES) {
            packet.values[numberOfValues++] = board->getValues()[i];
        }
        else {
            Serial.println("Too many values in done!");
        }
    }

    portExpander->select(5);

    HardwareSerial *serial = portExpander->getSerial();
    Adafruit_GPS gps(serial);

    gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);

    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("Reading GPS...");

    uint32_t started = millis();

    while (millis() - started < 10 * 1000) {
        while (serial->available()) {
            gps.read();

            if (gps.newNMEAreceived()) {
                if (gps.parse(gps.lastNMEA())) {
                    if (gps.fix) {
                        DEBUG_PRINTLN("Fix");

                        DateTime dateTime = DateTime(gps.year, gps.month, gps.year, gps.hour, gps.minute, gps.seconds);
                        uint32_t time = dateTime.unixtime();
                        SystemClock->set(time);

                        packet.time = time;
                        packet.battery = gauge != nullptr ? gauge->stateOfCharge() : 0;
                        packet.fk.kind = FK_PACKET_KIND_DATA_BOAT_SENSORS;
                        packet.latitude = gps.latitudeDegrees;
                        packet.longitude = gps.longitudeDegrees;
                        packet.altitude = gps.altitude;
                        packet.angle = gps.angle;
                        packet.speed = gps.speed;

                        logPacketLocally(numberOfValues);

                        if (handler != nullptr) {
                            handler->handleReading(&packet, numberOfValues);
                        }

                        board->takeReading();

                        return;
                    }
                    else {
                        DEBUG_PRINTLN("No fix");
                    }
                }
            }
        }
    }

    serial->end();
}

void LoggingAtlasSensorBoard::writePacket(Stream &stream, data_boat_packet_t *packet, size_t numberOfValues) {
    stream.print(packet->time);
    stream.print(",");
    stream.print(packet->battery);
    stream.print(",");
    stream.print(packet->latitude, 6);
    stream.print(",");
    stream.print(packet->longitude, 6);
    stream.print(",");
    stream.print(packet->altitude, 6);
    stream.print(",");
    stream.print(packet->angle);
    stream.print(",");
    stream.print(packet->speed);

    stream.print(packet->time);
    stream.print(",");
    stream.print(packet->battery);

    for (uint8_t i = 0; i < numberOfValues; ++i) {
        stream.print(",");
        stream.print(packet->values[i]);
    }

    stream.println();
}

void LoggingAtlasSensorBoard::logPacketLocally(size_t numberOfValues) {
    Queue queue;
    queue.enqueue((uint8_t *)&packet, sizeof(data_boat_packet_t));
    queue.startAtBeginning();

    File file = Logger::open(FK_SETTINGS_ATLAS_DATA_FILENAME);
    if (file) {
        writePacket(file, &packet, numberOfValues);
        file.close();
    }

    writePacket(Serial, &packet, numberOfValues);
}
