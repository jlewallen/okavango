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

class ParallelizedAtlasScientificSensors : public NonBlockingSerialProtocol {
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

    virtual bool tick();

    const float *getValues() {
        return values;
    }

    uint8_t getNumberOfValues() {
        return numberOfValues;
    }

    bool isDone() {
        return state == ParallelizedAtlasScientificSensorsState::Done;
    }

    void start(bool setupSerial = true) {
        state = ParallelizedAtlasScientificSensorsState::Start;
        if (setupSerial) {
            setup();
        }
        open();
    }

protected:
    void transition(ParallelizedAtlasScientificSensorsState newState);
    virtual bool handle(String reply);
};

#endif
