#include <Adafruit_GPS.h>

#include "LoggingAtlasSensorBoard.h"

LoggingAtlasSensorBoard::LoggingAtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *serialPortExpander, SensorBoard *sensorBoard) :
    AtlasSensorBoard(corePlatform, serialPortExpander, sensorBoard, nullptr, true) {
}

void LoggingAtlasSensorBoard::takeExtraReadings() {
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
                        dataBoatPacket.time = time;
                        dataBoatPacket.latitude = gps.latitudeDegrees;
                        dataBoatPacket.longitude = gps.longitudeDegrees;
                        dataBoatPacket.altitude = gps.altitude;
                        dataBoatPacket.angle = gps.angle;
                        dataBoatPacket.speed = gps.speed;
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

void LoggingAtlasSensorBoard::writePacket(Stream &stream, atlas_sensors_packet_t *packet) {
    stream.print(dataBoatPacket.latitude, 6);
    stream.print(",");
    stream.print(dataBoatPacket.longitude, 6);
    stream.print(",");
    stream.print(dataBoatPacket.altitude, 6);
    stream.print(",");
    stream.print(dataBoatPacket.angle);
    stream.print(",");
    stream.print(dataBoatPacket.speed);

    stream.print(dataBoatPacket.time);
    stream.print(",");
    stream.print(packet->battery);

    for (uint8_t i = 0; i < FK_ATLAS_SENSORS_PACKET_NUMBER_VALUES; ++i) {
        stream.print(",");
        stream.print(packet->values[i]);
    }

    stream.println();
}

void LoggingAtlasSensorBoard::doneReadingSensors(Queue *queue, atlas_sensors_packet_t *packet) {
}

