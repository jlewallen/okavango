#ifndef ATLAS_SCIENTIFIC_H
#define ATLAS_SCIENTIFIC_H

#include <Arduino.h>
#include "NonBlockingSerial.h"

enum class AtlasScientificBoardState {
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

class AtlasScientificBoard : public NonBlockingSerialProtocol {
private:
    const static int8_t MAX_VALUES = 4;
    AtlasScientificBoardState state = AtlasScientificBoardState::Start;
    float values[MAX_VALUES];
    uint8_t numberOfValues;
    bool disableSleep;

public:
    AtlasScientificBoard(bool disableSleep);

    virtual bool tick();

    const float *getValues() {
        return values;
    }

    uint8_t getNumberOfValues() {
        return numberOfValues;
    }

    bool isDone() {
        return state == AtlasScientificBoardState::Done;
    }

    void start(bool setupSerial = true) {
        state = AtlasScientificBoardState::Start;
        if (setupSerial) {
            setup();
        }
        open();
    }

protected:
    void transition(AtlasScientificBoardState newState);
    virtual bool handle(String reply);
};

#endif
