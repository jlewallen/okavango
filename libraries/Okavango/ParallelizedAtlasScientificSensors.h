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
    Waiting,
    Read0,
    Read1,
    LedsOff,
    Sleeping,
    Done
};

class ParallelizedAtlasScientificSensors : public NonBlockingSerialProtocol, public SensorBoard {
private:
    ParallelizedAtlasScientificSensorsState state = ParallelizedAtlasScientificSensorsState::Start;
    SerialPortExpander *serialPortExpander;
    float values[FK_ATLAS_SENSORS_PACKET_NUMBER_VALUES];
    uint32_t lastTransisitonAt;
    uint8_t numberOfRead0s;
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
