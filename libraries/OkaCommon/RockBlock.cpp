#include "RockBlock.h"
#include <Adafruit_SleepyDog.h>

#ifndef CUSTOM_ROCKBLOCK

#include <IridiumSBD.h>

RockBlock::RockBlock(uint8_t *buffer, size_t size) :
    buffer(buffer), size(size),
    NonBlockingSerialProtocol(10 * 1000, true, false),
    sendTries(0), signalTries(0) {
}

RockBlock::RockBlock(String message) :
    message(message), NonBlockingSerialProtocol(10 * 1000, true, false),
    sendTries(0), signalTries(0) {
}

class WatchdogCallbacks : public IridiumCallbacks  {
public:
    virtual void tick() override {
        Watchdog.reset();
        logPrinter.flush();
    }
};

IridiumSBD rockBlock(Serial2, PIN_ROCK_BLOCK, new WatchdogCallbacks());

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

    rockBlock.setPowerProfile(1);
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

                DEBUG_PRINT("Signal quality: ");
                DEBUG_PRINTLN((int32_t)signalQuality);

                uint8_t *data = buffer;
                size_t dataSize = size;
                if (data == nullptr) {
                    data = (uint8_t *)message.c_str();
                    dataSize = message.length();
                }
                error = rockBlock.sendSBDBinary(data, dataSize);
                if (error == ISBD_SUCCESS) {
                    success = true;
                    break;
                }

                DEBUG_PRINT("SB: send failed: ");
                DEBUG_PRINTLN(error);
            }
        }

        rockBlock.sleep();
        DEBUG_PRINTLN("Done");
    }
    else {
        DEBUG_PRINTLN("No RockBlock");
    }

    if (!success) {
        transition(RockBlockFailed);
    }
    else {
        transition(RockBlockDone);
    }
    return success;
}

bool RockBlock::handle(String reply) {
    return false;
}

#else

// Also, this kept trying after it sent.

RockBlock::RockBlock(String message) :
    message(message), NonBlockingSerialProtocol(10 * 1000, true, false),
    sendTries(0), signalTries(0) {
}

bool RockBlock::tick() {
    if (NonBlockingSerialProtocol::tick()) {
        return true;
    }

    if (getSendsCounter() == 5) {
        transition(RockBlockPowerOffBeforeFailed);
        return true;
    }

    switch (state) {
        case RockBlockStart: {
            transition(RockBlockPowerOn);

            Serial.println(message.length());
            break;
        }
        case RockBlockPowerOn: {
            pinMode(PIN_ROCK_BLOCK, OUTPUT);
            digitalWrite(PIN_ROCK_BLOCK, HIGH);

            transition(RockBlockConfigure0);
            break;
        }
        case RockBlockConfigure0: {
            sendCommand("AT");
            break;
        }
        case RockBlockConfigure1: {
            sendCommand("AT&K0");
            break;
        }
        case RockBlockConfigure2: {
            sendCommand("AT&K0");
            break;
        }
        case RockBlockPrepareMessage: {
            sendCommand("AT+SBDWT");
            break;
        }
        case RockBlockWriteMessage: {
            sendCommand(message.c_str());
            break;
        }
        case RockBlockSignalStrength: {
            if (signalTries++ > 10) {
                transition(RockBlockPowerOffBeforeFailed);
            }
            else {
                sendCommand("AT+CSQ");
            }
            break;
        }
        case RockBlockWaitForNetwork: {
            if (millis() - lastStateChange > 5000) {
                transition(RockBlockSignalStrength);
            }
            break;
        }
        case RockBlockSendMessage: {
            if (sendTries == 5) {
                transition(RockBlockPowerOffBeforeFailed);
            }
            else  {
                sendCommand("AT+SBDIX");
            }
            break;
        }
        case RockBlockPowerOffBeforeFailed: {
            pinMode(PIN_ROCK_BLOCK, OUTPUT);
            digitalWrite(PIN_ROCK_BLOCK, LOW);
            transition(RockBlockFailed);
            DEBUG_PRINTLN("Failed");
            break;
        }
        case RockBlockPowerOffBeforeDone: {
            pinMode(PIN_ROCK_BLOCK, OUTPUT);
            digitalWrite(PIN_ROCK_BLOCK, LOW);
            transition(RockBlockDone);
            DEBUG_PRINTLN("Done");
            break;
        }
        case RockBlockFailed: {
            break;
        }
        case RockBlockDone: {
            break;
        }
    }
    return true;
}

bool RockBlock::handle(String reply) {
    if (reply.length() > 0) {
        reply.trim();
        if (reply.length() > 0) {
            Serial.print(state);
            Serial.print(">");
            Serial.println(reply);
        }
    }
    if (reply.indexOf("OK") >= 0) {
        switch (state) {
            case RockBlockConfigure0: {
                transition(RockBlockConfigure1);
                break;
            }
            case RockBlockConfigure1: {
                transition(RockBlockConfigure2);
                break;
            }
            case RockBlockConfigure2: {
                transition(RockBlockPrepareMessage);
                break;
            }
            case RockBlockWriteMessage: {
                transition(RockBlockSignalStrength);
                break;
            }
            case RockBlockSignalStrength: {
                if (signalStrength > 0) {
                    Serial.println("Send");
                    transition(RockBlockSendMessage);
                }
                else {
                    Serial.println("Waiting...");
                    transition(RockBlockWaitForNetwork);
                }
                break;
            }
            case RockBlockSendMessage: {
                if (success) {
                    transition(RockBlockPowerOffBeforeDone);
                }
                else {
                    if (sendTries < 3) {
                        transition(RockBlockSignalStrength);
                    }
                    else {
                        transition(RockBlockPowerOffBeforeFailed);
                    }
                }
                break;
            }
        }
        return true;
    }
    else if (reply.indexOf("READY") == 0) {
        if (state == RockBlockPrepareMessage) {
            transition(RockBlockWriteMessage);
        }
        return true;
    }
    else if (reply.indexOf("+CSQ:") == 0) {
        if (state == RockBlockSignalStrength) {
            int8_t i = reply.indexOf(":");
            signalStrength = reply.substring(i + 1).toInt();
            return false;
        }
    }
    else if (reply.indexOf("+SBDIX:") == 0) {
        if (state == RockBlockSendMessage) {
            int8_t i = reply.indexOf(":");
            int8_t c = reply.indexOf(",");
            uint8_t status = reply.substring(i + 1, c).toInt();
            success = status >= 0 && status < 4;
            return false;
        }
    }
    else if (reply.indexOf("0") == 0) {
        if (state == RockBlockWriteMessage) {
            Serial.println("Message Written");
        }
        return false;
    }
    return false;
}

#endif
