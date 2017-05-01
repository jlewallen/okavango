#include "Platforms.h"
#include "AttachedGps.h"
#include "core.h"

AttachedGps::AttachedGps(HardwareSerial *serial, uint8_t pinGpsEnable) :
    gps(serial) {
}

void AttachedGps::setup() {
    platformSerial2Begin(9600);

    gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
    gps.sendCommand(PGCMD_ANTENNA);
}

bool AttachedGps::tick(data_boat_packet_t *packet) {
    if (Serial2.available()) {
        while (Serial2.available()) {
            char c = gps.read();
            Serial.print(c);
        }
    }
    if (gps.newNMEAreceived()) {
        if (gps.parse(gps.lastNMEA())) {
            if (gps.fix) {
                Serial.println("GOT NMEA");
                DateTime dateTime = DateTime(gps.year, gps.month, gps.year, gps.hour, gps.minute, gps.seconds);
                uint32_t time = dateTime.unixtime();
                SystemClock->set(time);
                packet->time = time;
                packet->latitude = gps.latitudeDegrees;
                packet->longitude = gps.longitudeDegrees;
                packet->altitude = gps.altitude;
                packet->angle = gps.angle;
                packet->speed = gps.speed;
                return true;
            }
        }
    }
    return false;
}
