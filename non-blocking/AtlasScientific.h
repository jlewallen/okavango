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
    AtlasScientificBoardState state = Start;

public:
    AtlasScientificBoard(byte rx, byte tx);

    virtual bool tick();

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
