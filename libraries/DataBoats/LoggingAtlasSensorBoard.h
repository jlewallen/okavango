#ifndef LOGGING_ATLAS_SENSOR_BOARD_INCLUDED
#define LOGGING_ATLAS_SENSOR_BOARD_INCLUDED

#include "core.h"
#include "SerialPortExpander.h"
#include "AtlasSensorBoard.h"

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

#endif
