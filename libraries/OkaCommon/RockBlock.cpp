#include "RockBlock.h"

RockBlock::RockBlock(String message) :
    message(message), NonBlockingSerialProtocol(true, false), tries(0) {
}

bool RockBlock::tick() {
    if (NonBlockingSerialProtocol::tick()) {
        return true;
    }

    switch (state) {
        case Start: {
            available = false;
            sendCommand("AT");
            break;
        }
        case SignalStrength: {
            sendCommand("AT+CSQ");
            break;
        }
        case WaitForNetwork: {
            if (millis() - lastStateChange > 2000) {
                transition(SignalStrength);
            }
            break;
        }
        case PrepareMessage: {
            sendCommand("AT+SBDWT=0,1.0,2.0,3.0");
            break;
        }
        case SendMessage: {
            sendCommand("AT+SBDIX");
            break;
        }
        case PowerOffBeforeFailed: {
            break;
        }
        case PowerOffBeforeDone: {
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
            case Start: {
                transition(SignalStrength);
                break;
            }
            case Power: {
                break;
            }
            case SignalStrength: {
                if (available) {
                    transition(PrepareMessage);
                }
                else {
                    transition(WaitForNetwork);
                }
                break;
            }
            case PrepareMessage: {
                transition(SendMessage);
                break;
            }
            case SendMessage: {
                transition(Done);
                break;
            }
            case PowerOffBeforeFailed: {
                transition(Failed);
                break;
            }
            case PowerOffBeforeDone: {
                transition(Done);
                break;
            }
            case Done: {
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


