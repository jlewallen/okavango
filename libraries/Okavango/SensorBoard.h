#ifndef SENSOR_BOARD_H
#define SENSOR_BOARD_H

#include "protocol.h"

class SensorBoard {
public:
    virtual void start() = 0;
    virtual bool tick() = 0;
    virtual void takeReading() = 0;
    virtual bool isDone() = 0;
    virtual const float *getValues() = 0;
    virtual uint8_t getNumberOfValues() = 0;
};

#endif
