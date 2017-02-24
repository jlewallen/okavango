#include "RockBlock.h"
#include <Adafruit_SleepyDog.h>

#include <IridiumSBD.h>

RockBlock::RockBlock(RockBlockMessages *messages, uint8_t *buffer, size_t size) :
    NonBlockingSerialProtocol(10 * 1000, true, false),
    messages(messages),
    txBuffer(buffer), txSize(size), rxSize(0),
    sendTries(0), signalTries(0) {
}

class WatchdogCallbacks : public IridiumCallbacks  {
public:
    virtual void tick() override {
        Watchdog.reset();
        logPrinter.flush();
    }
};

IridiumSBD rockBlock(Serial2, -1, new WatchdogCallbacks());

bool RockBlock::tick() {
    if (Serial) {
        rockBlock.attachConsole(Serial);
        rockBlock.attachDiags(Serial);
    }
    else {
        rockBlock.attachConsole(logPrinter);
        rockBlock.attachDiags(logPrinter);
    }
    bool success = false;

    rockBlock.setPowerProfile(0);
    if (rockBlock.begin() == ISBD_SUCCESS) {
        for (uint8_t i = 0; i < 2; ++i) {
            int signalQuality = 0;
            int32_t error = rockBlock.getSignalQuality(signalQuality);
            if (error != ISBD_SUCCESS) {
                DEBUG_PRINT("RB: getSignalQuality failed ");
                DEBUG_PRINTLN(error);
            }
            else {
                Watchdog.reset();

                DEBUG_PRINT("RB: Signal quality: ");
                DEBUG_PRINTLN((int32_t)signalQuality);

                rxSize = sizeof(rxBuffer);
                error = rockBlock.sendReceiveSBDBinary(txBuffer, txSize, rxBuffer, rxSize);
                if (error == ISBD_SUCCESS) {
                    handleReceivedMessage();

                    while (rockBlock.getWaitingMessageCount() > 0) {
                        rxSize = sizeof(rxBuffer);
                        error = rockBlock.sendReceiveSBDBinary(nullptr, 0, rxBuffer, rxSize);
                        if (error != ISBD_SUCCESS) {
                            break;
                        }

                        handleReceivedMessage();
                    }

                    success = true;
                    break;
                }

                DEBUG_PRINT("RB: send failed: ");
                DEBUG_PRINTLN(error);
            }
        }

        rockBlock.sleep();
        DEBUG_PRINTLN("RB: Done");
    }
    else {
        DEBUG_PRINTLN("RB: No RockBlock");
    }

    if (!success) {
        transition(RockBlockFailed);
    }
    else {
        transition(RockBlockDone);
    }
    return success;
}

void RockBlock::handleReceivedMessage() {
    if (rxSize == 0) {
        return;
    }

    // TODO: What do we wanna tell them?
    rxBuffer[rxSize] = 0;
    char *ptr = (char *)rxBuffer;
    String message((char *)rxBuffer);

    DEBUG_PRINTLN("");
    DEBUG_PRINT("Received: ");
    DEBUG_PRINTLN(message);

    messages->onMessage(message);
}
