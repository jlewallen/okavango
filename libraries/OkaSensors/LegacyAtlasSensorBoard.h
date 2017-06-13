#ifndef LEGACY_ATLAS_SENSOR_BOARD_H
#define LEGACY_ATLAS_SENSOR_BOARD_H

#include <Adafruit_BME280.h>
#include <OneWire.h>

#include "protocol.h"
#include "Logger.h"
#include "Queue.h"
#include "AtlasSensorBoard.h"

#define FK_ATLAS_SENSORS_FIELD_ORP                           0
#define FK_ATLAS_SENSORS_FIELD_PH                            1
#define FK_ATLAS_SENSORS_FIELD_DO                            2
#define FK_ATLAS_SENSORS_FIELD_EC                            3
#define FK_ATLAS_SENSORS_FIELD_TDS                           4
#define FK_ATLAS_SENSORS_FIELD_SALINITY                      5
#define FK_ATLAS_SENSORS_FIELD_SG                            6

#define FK_ATLAS_SENSORS_FIELD_HUMIDITY                      7
#define FK_ATLAS_SENSORS_FIELD_PRESSURE                      8
#define FK_ATLAS_SENSORS_FIELD_AIR_TEMPERATURE               9
#define FK_ATLAS_SENSORS_FIELD_WATER_TEMPERATURE             10

class LegacyAtlasSensorBoard : public AtlasSensorBoard {
private:
    Adafruit_BME280 bme;
    atlas_sensors_packet_t packet;
    size_t packetValueIndex = 0;
    bool continuous = false;
    bool displayedWaterTemperature = false;

public:
    LegacyAtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *portExpander, SensorBoard *board, FuelGauge *gauge, bool continuous);

public:
    virtual bool tick() override;

protected:
    virtual void done(SensorBoard *board) override;

private:
    void takeExtraReadings();
    void populatePacket(SensorBoard *board);
    void logPacketLocally();
    void writePacket(Stream &stream, atlas_sensors_packet_t *packet);
    float getWaterTemperature();

};

#endif
