#ifndef LORA_ATLAS_BOARD_INCLUDED
#define LORA_ATLAS_BOARD_INCLUDED

#include "LegacyAtlasSensorBoard.h"
#include "LoraRadio.h"
#include "FuelGauge.h"

class LoraAtlasSensorBoard : public LegacyAtlasSensorBoard {
public:
    LoraAtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *serialPortExpander, SensorBoard *sensorBoard, FuelGauge *gauge);

public:
    void done(SensorBoard *board) override;
    void tryAndSendLocalQueue();
};

#endif
