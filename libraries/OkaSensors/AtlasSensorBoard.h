#ifndef ATLAS_SENSOR_BOARD_H
#define ATLAS_SENSOR_BOARD_H

#include "Platforms.h"
#include "core.h"
#include "FuelGauge.h"
#include "SensorBoard.h"
#include "SerialPortExpander.h"
#include "ParallelizedAtlasScientificSensors.h"

class AtlasSensorBoard {
protected:
    CorePlatform *corePlatform;
    SerialPortExpander *portExpander;
    SensorBoard *board;
    FuelGauge *gauge;

public:
    AtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *portExpander, SensorBoard *board, FuelGauge *gauge);

public:
    virtual bool tick();
    virtual void setup();

protected:
    virtual void done(SensorBoard *board) = 0;

};

#endif
