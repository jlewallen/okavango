#include "RockBlock.h"

RockBlock::RockBlock(String message) :
    message(message), NonBlockingSerialProtocol(10 * 1000, true, false), tries(0) {
}

bool RockBlock::tick() {
    if (NonBlockingSerialProtocol::tick()) {
        return true;
    }

    switch (state) {
        case RockBlockStart: {
            available = false;
            transition(RockBlockPowerOn);
            break;
        }
        case RockBlockPowerOn: {
            pinMode(PIN_ROCK_BLOCK, OUTPUT);
            digitalWrite(PIN_ROCK_BLOCK, HIGH);

            transition(RockBlockConfigure);
            break;
        }
        case RockBlockConfigure: {
            sendCommand("AT");
            break;
        }
        case RockBlockSignalStrength: {
            if (tries++ > 10) {
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
        case RockBlockPrepareMessage: {
            if (message.length() > 0) {
                String command = "AT+SBDWT=" + message;
                sendCommand(command.c_str());
            }
            else {
                DEBUG_PRINTLN("No message");
                transition(RockBlockPowerOffBeforeDone);
            }
            break;
        }
        case RockBlockSendMessage: {
            sendCommand("AT+SBDIX");
            break;
        }
        case RockBlockPowerOffBeforeFailed: {
            pinMode(PIN_ROCK_BLOCK, OUTPUT);
            digitalWrite(PIN_ROCK_BLOCK, LOW);
            transition(RockBlockFailed);
            break;
        }
        case RockBlockPowerOffBeforeDone: {
            pinMode(PIN_ROCK_BLOCK, OUTPUT);
            digitalWrite(PIN_ROCK_BLOCK, LOW);
            transition(RockBlockDone);
            break;
        }
        case RockBlockFailed: {
            DEBUG_PRINTLN("Failed");
            break;
        }
        case RockBlockDone: {
            DEBUG_PRINTLN("Done");
            break;
        }
    }
    return true;
}

bool RockBlock::handle(String reply) {
    if (reply.length() > 0) {
        reply.trim();
        if (reply.length() > 0) {
            Serial.print(">");
            Serial.println(reply);
        }
    }
    if (reply.indexOf("OK") >= 0) {
        switch (state) {
            case RockBlockConfigure: {
                transition(RockBlockSignalStrength);
                break;
            }
            case RockBlockSignalStrength: {
                if (available) {
                    transition(RockBlockPrepareMessage);
                }
                else {
                    transition(RockBlockWaitForNetwork);
                }
                break;
            }
            case RockBlockPrepareMessage: {
                transition(RockBlockSendMessage);
                break;
            }
            case RockBlockSendMessage: {
                transition(RockBlockPowerOffBeforeDone);
                break;
            }
        }
        return true;
    }
    else if (reply.indexOf("+CSQ:") == 0) {
        int8_t i = reply.indexOf(":");
        uint8_t strength = reply.substring(i + 1).toInt();
        if (strength > 0) {
            available = true;
        }
        else {
            available = false;
        }
        return false;
    }
    return false;
}


