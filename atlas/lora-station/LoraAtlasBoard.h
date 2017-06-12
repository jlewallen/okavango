#ifndef LORA_ATLAS_BOARD_INCLUDED
#define LORA_ATLAS_BOARD_INCLUDED

#include "AtlasSensorBoard.h"
#include "LoraRadio.h"
#include "FuelGauge.h"

class LoraAtlasSensorBoard : public AtlasSensorBoard {
public:
    LoraAtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *serialPortExpander, SensorBoard *sensorBoard, FuelGauge *gauge);

public:
    void doneReadingSensors(Queue *queue, atlas_sensors_packet_t *packet) override;
    void tryAndSendLocalQueue(Queue *queue);
};

#endif
