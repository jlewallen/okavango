#include <String.h>
#include "AtlasScientific.h"

const char *CMD_STATUS = "STATUS";
const char *CMD_LED_ON = "L,1";
const char *CMD_LED_OFF = "L,0";
const char *CMD_CONTINUOUS_OFF = "C,0";
const char *CMD_SLEEP = "SLEEP";
const char *CMD_READ = "R";

AtlasScientificBoard::AtlasScientificBoard() {
}

bool AtlasScientificBoard::tick() {
    if (NonBlockingSerialProtocol::tick()) {
        return true;
    }
    switch (state) {
        case Start: {
            hasValue = false;
            transition(Status0);
            break;
        }
        case Status0: {
            sendCommand(CMD_STATUS);
            break;
        }
        case Status1: {
            sendCommand(CMD_STATUS);
            break;
        }
        case Leds: {
            sendCommand(CMD_LED_ON);
            break;
        }
        case Configure: {
            sendCommand(CMD_CONTINUOUS_OFF);
            break;
        }
        case Read0: {
            sendCommand(CMD_READ);
            break;
        }
        case Read1: {
            sendCommand(CMD_READ);
            break;
        }
        case Read2: {
            sendCommand(CMD_READ);
            break;
        }
        case Sleeping: {
            sendCommand(CMD_SLEEP);
            close();
            transition(Done);
            break;
        }
        case Done: {
            return false;
        }
    }
    return true;
}

uint8_t numberOfOccurences(String &str, char chr) {
    uint8_t number = 0;
    for (uint16_t i = 0; i < str.length(); ++i) {
        if (str[i] == chr) {
            number++;
        }
    }

    return number;
}

bool AtlasScientificBoard::areWeDoneReading(String &buffer, char newChar) {
    if (state == Read0 || state == Read1 || state == Read2) {
        return numberOfOccurences(buffer, '\r') == 2;
    }

    return NonBlockingSerialProtocol::areWeDoneReading(buffer, newChar);
}

bool AtlasScientificBoard::handle(String reply) {
    Serial.println(reply);
    if (reply.indexOf("*") >= 0) {
        switch (state) {
            case Status0: {
                transition(Status1);
                break;
            }
            case Status1: {
                transition(Leds);
                break;
            }
            case Leds: {
                transition(Configure);
                break;
            }
            case Configure: {
                transition(Read0);
                break;
            }
            case Read0: {
                transition(Read1);
                break;
            }
            case Read1: {
                transition(Read2);
                break;
            }
            case Read2: {
                transition(Sleeping);
                int16_t first = reply.indexOf('\n');
                if (first >= 0) {
                    int16_t second = reply.indexOf('\n', first + 1);
                    if (second >= first) {
                        String part = reply.substring(first, second - first);
                        value = part.toFloat();
                        hasValue = true;
                    }
                }
                break;
            }
        }
        return false;
    }
    return true;
}

