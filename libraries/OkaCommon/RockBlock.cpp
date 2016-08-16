#include "RockBlock.h"
#include <Adafruit_SleepyDog.h>

#ifndef CUSTOM_ROCKBLOCK

#include <IridiumSBD.h>

RockBlock::RockBlock(String message) :
    message(message), NonBlockingSerialProtocol(10 * 1000, true, false),
    sendTries(0), signalTries(0) {
}

class WatchdogCallbacks : public IridiumCallbacks  {
public:
    virtual void tick() override {
        Watchdog.reset();
    }
};

bool RockBlock::tick() {
    IridiumSBD rockBlock(Serial1, PIN_ROCK_BLOCK, new WatchdogCallbacks());

    // rockBlock.sleep();
    rockBlock.attachConsole(Serial);
    rockBlock.setPowerProfile(1);
    rockBlock.begin();

    int signalQuality = 0;
    int32_t error = rockBlock.getSignalQuality(signalQuality);
    if (error != 0) {
        Serial.print("RB: getSignalQuality failed");
        Serial.println(error);
        transition(RockBlockFailed);
        return false;
    }

    Watchdog.reset();

    Serial.print("Signal quality: ");
    Serial.println(signalQuality);

    error = rockBlock.sendSBDText(message.c_str());
    if (error != 0) {
        Serial.print("SB: send failed: ");
        Serial.println(error);
        transition(RockBlockFailed);
        return false;
    }

    // rockBlock.sleep();

    Serial.println("Done");
    transition(RockBlockDone);
    return true;
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
