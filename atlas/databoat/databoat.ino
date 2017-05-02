#include <SPI.h>
#include <SD.h>
#include <Adafruit_SleepyDog.h>
#include <Adafruit_GPS.h>

#include "Platforms.h"
#include "core.h"
#include "AtlasSensorBoard.h"
#include "DataBoat.h"

const uint8_t PIN_SPE_ISO_SEL0 = 14;
const uint8_t PIN_SPE_ISO_SEL1 = 15;
const uint8_t PIN_SPE_SEL0 = 16;
const uint8_t PIN_SPE_SEL1 = 17;

CorePlatform corePlatform;
SingleSerialPortExpander speIsolated(PIN_SPE_ISO_SEL0, PIN_SPE_ISO_SEL1, ConductivityConfig::None, &Serial2, 4);
SingleSerialPortExpander speNormal(PIN_SPE_SEL0, PIN_SPE_SEL1, ConductivityConfig::None, &Serial1, 1);
DualSerialPortExpander serialPortExpander(&speIsolated, &speNormal);
ParallelizedAtlasScientificSensors sensorBoard(&serialPortExpander, true);
ZeroSystemClock Clock;

class LoggingAtlasSensorBoard : public AtlasSensorBoard {
private:
    data_boat_packet_t dataBoatPacket;

public:
    LoggingAtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *serialPortExpander, SensorBoard *sensorBoard);

public:
    void doneReadingSensors(Queue *queue, atlas_sensors_packet_t *packet) override;

protected:
    void takeExtraReadings() override;
    void writePacket(Stream &stream, atlas_sensors_packet_t *packet) override;
};

LoggingAtlasSensorBoard::LoggingAtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *serialPortExpander, SensorBoard *sensorBoard) :
    AtlasSensorBoard(corePlatform, serialPortExpander, sensorBoard, nullptr, true) {
}

void LoggingAtlasSensorBoard::takeExtraReadings() {
    serialPortExpander.select(5);

    HardwareSerial *serial = serialPortExpander.getSerial();
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
    stream.print(dataBoatPacket.latitude);
    stream.print(",");
    stream.print(dataBoatPacket.longitude);
    stream.print(",");
    stream.print(dataBoatPacket.altitude);
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

LoggingAtlasSensorBoard atlasSensorBoard(&corePlatform, &serialPortExpander, &sensorBoard);

void setup() {
    Serial.begin(115200);

#ifdef WAIT_FOR_SERIAL
    while (!Serial) {
        delay(100);
        if (millis() > WAIT_FOR_SERIAL) {
            break;
        }
    }
#endif

    Serial.println("Begin");

    corePlatform.setup(4, PIN_RFM95_CS, PIN_RFM95_RST, true);

    serialPortExpander.setup();

    Serial.println("Loop");
}

void loop() {
    atlasSensorBoard.tick();
}

// vim: set ft=cpp:
