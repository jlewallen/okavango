#ifndef ROCKBLOCK_H
#define ROCKBLOCK_H
#include "Platforms.h"

#include "NonBlockingSerial.h"

#define RB_RECEIVE_BUFFER_LENGTH 256

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

class RockBlockMessages {
public:
    virtual void onMessage(String message) = 0;
};

class RockBlock : public NonBlockingSerialProtocol {
private:
    RockBlockState state = RockBlockStart;
    RockBlockMessages *messages;
    uint32_t lastStateChange;
    uint8_t signalTries;
    uint8_t sendTries;
    uint8_t signalStrength;
    bool success;
    uint8_t *txBuffer;
    size_t txSize;
    uint8_t rxBuffer[RB_RECEIVE_BUFFER_LENGTH];
    size_t rxSize;

public:
    RockBlock(RockBlockMessages *messages, uint8_t *buffer, size_t size);

    virtual bool tick();

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

private:
    virtual void handleReceivedMessage();

};

#endif
