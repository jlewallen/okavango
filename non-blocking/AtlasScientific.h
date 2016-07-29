#ifndef ATLAS_SCIENTIFIC_H
#define ATLAS_SCIENTIFIC_H

#include <Arduino.h>
#include "NonBlockingSerial.h"
#include "Tickable.h"

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

class AtlasScientificBoard : public NonBlockingSerialProtocol, public Tickable {
private:
    const static int8_t MAX_VALUES = 4;
    AtlasScientificBoardState state = Start;
    float values[MAX_VALUES];
    int8_t numberOfValues;

public:
    AtlasScientificBoard();

    virtual bool tick();

    const float *getValues() {
        return values;
    }

    int8_t getNumberOfValues() {
        return numberOfValues;
    }

    bool isDone() {
        return state == Done;
    }

    void start() {
        state = Start;
        setup();
        open();
    }

protected:
    void transition(AtlasScientificBoardState newState) {
        state = newState;
    }

    virtual bool handle(String reply);
};

#endif
