#ifndef ATLAS_SCIENTIFIC_H
#define ATLAS_SCIENTIFIC_H

#include <Arduino.h>
#include "SensorBoard.h"
#include "NonBlockingSerial.h"
#include "SerialPortExpander.h"

enum class AtlasScientificBoardState {
    Start,
    DisableContinuousReading,
    ConfigureResponse,
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

class AtlasScientificBoard : public NonBlockingSerialProtocol, public SensorBoard {
private:
    AtlasScientificBoardState state = AtlasScientificBoardState::Start;
    SerialPortExpander *serialPortExpander;
    float values[FK_ATLAS_SENSORS_PACKET_NUMBER_VALUES];
    uint8_t numberOfValues;
    bool disableSleep;

public:
    AtlasScientificBoard(SerialPortExpander *serialPortExpander, bool disableSleep);

    virtual bool tick() override;

    virtual const float *getValues() override {
        return values;
    }

    virtual uint8_t getNumberOfValues() override {
        return numberOfValues;
    }

    virtual bool isDone() override {
        return state == AtlasScientificBoardState::Done;
    }

    virtual void start() override {
        state = AtlasScientificBoardState::Start;
        setSerial(serialPortExpander->getSerial());
        open();
    }

protected:
    void transition(AtlasScientificBoardState newState);
    virtual NonBlockingHandleStatus handle(String reply);
};

#endif
