#ifndef ATLAS_SCIENTIFIC_H
#define ATLAS_SCIENTIFIC_H

#include <Arduino.h>
#include "NonBlockingSerial.h"

enum AtlasScientificBoardState {
    Start,
    Status0,
    Status1,
    Leds,
    Configure,
    Read0,
    Read1,
    Read2,
    Sleeping,
    Done
};

class AtlasScientificBoard : public NonBlockingSerialProtocol {
private:
    const static int8_t MAX_VALUES = 4;
    AtlasScientificBoardState state = Start;
    float values[MAX_VALUES];
    uint8_t numberOfValues;

public:
    AtlasScientificBoard();

    virtual bool tick();

    const float *getValues() {
        return values;
    }

    uint8_t getNumberOfValues() {
        return numberOfValues;
    }

    bool isDone() {
        return state == Done;
    }

    void start(bool setupSerial = true) {
        state = Start;
        if (setupSerial) {
            setup();
        }
        open();
    }

protected:
    void transition(AtlasScientificBoardState newState) {
        state = newState;
    }

    virtual bool handle(String reply);
};

#endif
