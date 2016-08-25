#ifndef PARALLIZED_ATLAS_SCIENTIFICS_SENSORS_H
#define PARALLIZED_ATLAS_SCIENTIFICS_SENSORS_H

#include "SensorBoard.h"
#include "SerialPortExpander.h"
#include "NonBlockingSerial.h"

enum class ParallelizedAtlasScientificSensorsState {
    Start,
    Status0,
    Status1,
    LedsOn,
    Configure,
    Read0,
    Read1,
    Read2,
    LedsOff,
    Sleeping,
    Done
};

class ParallelizedAtlasScientificSensors : public NonBlockingSerialProtocol, public SensorBoard {
private:
    const static int8_t MAX_VALUES = 4;
    ParallelizedAtlasScientificSensorsState state = ParallelizedAtlasScientificSensorsState::Start;
    SerialPortExpander *serialPortExpander;
    float values[MAX_VALUES];
    uint8_t numberOfValues;
    uint8_t portNumber;
    bool disableSleep;

public:
    ParallelizedAtlasScientificSensors(SerialPortExpander *serialPortExpander, bool disableSleep);

    virtual bool tick() override;

    virtual const float *getValues() override {
        return values;
    }

    virtual uint8_t getNumberOfValues() override {
        return numberOfValues;
    }

    virtual bool isDone() override {
        return state == ParallelizedAtlasScientificSensorsState::Done;
    }

    virtual void start() override {
        state = ParallelizedAtlasScientificSensorsState::Start;
        setSerial(serialPortExpander->getSerial());
        open();
    }

protected:
    void transition(ParallelizedAtlasScientificSensorsState newState);
    virtual bool handle(String reply);
};

#endif
