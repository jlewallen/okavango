#ifndef ATLAS_SENSOR_BOARD_H
#define ATLAS_SENSOR_BOARD_H

#include <Adafruit_BME280.h>
#include <OneWire.h>

#include "Platforms.h"

#include "core.h"
#include "protocol.h"
#include "network.h"
#include "Logger.h"

#include "AtlasScientific.h"
#include "SerialPortExpander.h"

/**
 * A board containing primarily Atlas sensors and a few others, including water temperature.
 */
class AtlasSensorBoard {
private:
    CorePlatform *corePlatform;
    SerialPortExpander portExpander;
    AtlasScientificBoard board;
    Adafruit_BME280 bme;
    atlas_sensors_packet_t packet;
    uint8_t packetValueIndex = 0;

public:
    AtlasSensorBoard(CorePlatform *corePlatform);

public:
    bool tick();
    void setup();
    virtual void doneReadingSensors(Queue *queue);

private:
    float getWaterTemperature();

    void populatePacket();
    void logPacketLocally();
};

#endif
