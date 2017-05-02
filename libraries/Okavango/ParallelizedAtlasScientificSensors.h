#ifndef PARALLIZED_ATLAS_SCIENTIFICS_SENSORS_H
#define PARALLIZED_ATLAS_SCIENTIFICS_SENSORS_H

#include "SensorBoard.h"
#include "SerialPortExpander.h"
#include "NonBlockingSerial.h"

enum class ParallelizedAtlasScientificSensorsState {
    Start,
    DisableContinuousReading,
    ConfigureResponse,
    Status0,
    Status1,
    LedsOn,
    Configure,
    Waiting,
    Read0,
    LedsOnBeforeRead,
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
    uint8_t hasPortFailed[8];
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

    virtual bool isStartingFakeReads() override {
        return portNumber == 0 && state == ParallelizedAtlasScientificSensorsState::Read0;
    }

    virtual void takeReading() override {
        portNumber = 0;
        numberOfValues = 0;
        numberOfRead0s = 0;
        for (uint8_t i = 0; i < serialPortExpander->getNumberOfPorts(); ++i) {
            hasPortFailed[i] = 0;
        }
        state = ParallelizedAtlasScientificSensorsState::LedsOnBeforeRead;
        serialPortExpander->select(0);
        setSerial(serialPortExpander->getSerial());
        open();
    }

    virtual void start() override {
        state = ParallelizedAtlasScientificSensorsState::Start;
        serialPortExpander->select(0);
        setSerial(serialPortExpander->getSerial());
        open();
    }

protected:
    void transition(ParallelizedAtlasScientificSensorsState newState);
    virtual NonBlockingHandleStatus handle(String reply);
};

#endif
