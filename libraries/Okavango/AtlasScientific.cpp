#include <String.h>
#include "Platforms.h"
#include "AtlasScientific.h"

const char *CMD_STATUS = "STATUS";
const char *CMD_LED_ON = "L,1";
const char *CMD_LED_OFF = "L,0";
const char *CMD_CONTINUOUS_OFF = "C,0";
const char *CMD_SLEEP = "SLEEP";
const char *CMD_READ = "R";

AtlasScientificBoard::AtlasScientificBoard(bool disableSleep) :
    disableSleep(disableSleep) {
}

void AtlasScientificBoard::transition(AtlasScientificBoardState newState) {
    state = newState;
    clearSendsCounter();
}


bool AtlasScientificBoard::tick() {
    if (NonBlockingSerialProtocol::tick()) {
        return true;
    }
    if (getSendsCounter() == 5) {
        transition(Done);
        return true;
    }
    switch (state) {
        case Start: {
            numberOfValues = 0;
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
        case LedsOn: {
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
        case LedsOff: {
            sendCommand(CMD_LED_OFF);
            break;
        }
        case Sleeping: {
            sendCommand(CMD_SLEEP);
            break;
        }
        case Done: {
            DEBUG_PRINTLN("DONE");
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

String getFirstLine(String &str) {
    str.trim();

    uint8_t cr = str.indexOf('\r');
    if (cr >= 0) return str.substring(0, cr + 1);

    uint8_t nl = str.indexOf('\n');
    if (nl >= 0) return str.substring(0, nl + 1);
    return str;
}

bool AtlasScientificBoard::handle(String reply) {
    if (reply.indexOf("*") >= 0) {
        if (reply.length() > 0) {
            DEBUG_PRINT(state);
            DEBUG_PRINT(">");
            DEBUG_PRINTLN(reply);
        }

        switch (state) {
            case Status0: {
                transition(Status1);
                break;
            }
            case Status1: {
                transition(LedsOn);
                break;
            }
            case LedsOn: {
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
                int8_t position = 0;
                numberOfValues = 0;

                String firstLine = getFirstLine(reply);

                // DEBUG_PRINT("'");
                // DEBUG_PRINT(firstLine);
                // DEBUG_PRINTLN("'");

                while (true) {
                    int16_t index = firstLine.indexOf(',', position);
                    if (index < 0) {
                        index = firstLine.indexOf('\r', position);
                    }
                    if (index < 0) {
                        index = firstLine.indexOf('\n', position);
                    }
                    if (index > position && numberOfValues < MAX_VALUES) {
                        String part = firstLine.substring(position, index);
                        values[numberOfValues++] = part.toFloat();
                        position = index + 1;

                        // DEBUG_PRINT("Parse: '");
                        // DEBUG_PRINT(part);
                        // DEBUG_PRINTLN("'");
                    }
                    else {
                        break;
                    }
                }

                if (disableSleep) {
                    transition(LedsOff);
                }
                else {
                    transition(Sleeping);
                }

                break;
            }
            case LedsOff: {
                transition(Done);
                break;
            }
            case Sleeping: {
                transition(Done);
                close();
                break;
            }
        }
        return true;
    }
    return false;
}
