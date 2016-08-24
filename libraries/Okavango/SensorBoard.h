#ifndef SENSOR_BOARD_H
#define SENSOR_BOARD_H

class SensorBoard {
public:
    virtual bool tick() = 0;
    virtual bool isDone() = 0;
    virtual void start(bool setupSerial = true) = 0;
    virtual const float *getValues() = 0;
    virtual uint8_t getNumberOfValues() = 0;
    virtual void setSerial(SerialType *newSerial);
};

#endif
