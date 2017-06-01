#ifndef SENSOR_BOARD_H
#define SENSOR_BOARD_H

#define FK_ATLAS_BOARD_MAXIMUM_NUMBER_VALUES   (1 + 1 + 1 + 1 + 3)

class SensorBoard {
public:
    virtual void start() = 0;
    virtual bool tick() = 0;
    virtual void takeReading() = 0;
    virtual bool isDone() = 0;
    virtual const float *getValues() = 0;
    virtual uint8_t getNumberOfValues() = 0;
    virtual bool isStartingFakeReads() {
        return false;
    }
};

#endif
