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
    AtlasScientificBoardState state = Start;
    float value;
    bool hasValue;

public:
    AtlasScientificBoard(byte rx, byte tx);

    virtual bool tick();

    float getValue() {
        return value;
    }

    bool getHasValue() {
        return hasValue;
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

    virtual bool areWeDoneReading(String &buffer, char newChar);
    virtual void handle(String reply);
};

class PhBoard : public AtlasScientificBoard {
public:
    PhBoard(byte rx, byte tx);
};

#endif
