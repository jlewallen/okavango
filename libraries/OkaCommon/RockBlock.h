#ifndef ROCKBLOCK_H
#define ROCKBLOCK_H
#include "Platforms.h"

#include "NonBlockingSerial.h"

enum RockBlockState {
    Start,
    Power,
    SignalStrength,
    WaitForNetwork,
    PrepareMessage,
    SendMessage,
    PowerOffBeforeFailed,
    PowerOffBeforeDone,
    Failed,
    Done
};

class RockBlock : public NonBlockingSerialProtocol {
private:
    RockBlockState state = Start;
    uint32_t lastStateChange;
    uint8_t tries;
    bool available;
    String message;

public:
    RockBlock(String message);

    virtual bool tick();
    virtual bool handle(String reply);

    void transition(RockBlockState newState) {
        state = newState;
        lastStateChange = millis();
    }

    bool isDone() {
        return state == Done;
    }

    bool isFailed() {
        return state == Failed;
    }

};

#endif
