#ifndef ROCKBLOCK_H
#define ROCKBLOCK_H
#include "Platforms.h"

#include "NonBlockingSerial.h"

enum RockBlockState {
    RockBlockStart = 0,
    RockBlockPowerOn,
    RockBlockConfigure0,
    RockBlockConfigure1,
    RockBlockConfigure2,
    RockBlockPrepareMessage,
    RockBlockWriteMessage,
    RockBlockSignalStrength,
    RockBlockWaitForNetwork,
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
    uint8_t signalTries;
    uint8_t sendTries;
    uint8_t signalStrength;
    bool success;
    String message;
    uint8_t *buffer;
    size_t size;

public:
    RockBlock(uint8_t *buffer, size_t size);
    RockBlock(String message);

    virtual bool tick();
    virtual bool handle(String reply);

    void transition(RockBlockState newState) {
        state = newState;
        lastStateChange = millis();
        signalStrength = 0;
        clearSendsCounter();
    }

    bool isDone() {
        return state == RockBlockDone;
    }

    bool isFailed() {
        return state == RockBlockFailed;
    }

};

#endif
