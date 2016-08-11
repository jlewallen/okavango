#ifndef ROCKBLOCK_H
#define ROCKBLOCK_H
#include "Platforms.h"

#include "NonBlockingSerial.h"

enum RockBlockState {
    RockBlockStart,
    RockBlockPowerOn,
    RockBlockConfigure,
    RockBlockSignalStrength,
    RockBlockWaitForNetwork,
    RockBlockPrepareMessage,
    RockBlockSendMessage,
    RockBlockPowerOffBeforeFailed,
    RockBlockPowerOffBeforeDone,
    RockBlockFailed,
    RockBlockDone
};

class RockBlock : public NonBlockingSerialProtocol {
private:
    RockBlockState state = RockBlockStart;
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
        return state == RockBlockDone;
    }

    bool isFailed() {
        return state == RockBlockFailed;
    }

};

#endif
