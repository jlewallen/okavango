#include <string.h>
#include "Platforms.h"
#include "AtlasScientific.h"

const char *CMD_RESPONSE1 = "RESPONSE,1";
const char *CMD_STATUS = "STATUS";
const char *CMD_LED_ON = "L,1";
const char *CMD_LED_OFF = "L,0";
const char *CMD_CONTINUOUS_OFF = "C,0";
const char *CMD_SLEEP = "SLEEP";
const char *CMD_READ = "R";

AtlasScientificBoard::AtlasScientificBoard(SerialPortExpander *serialPortExpander, bool disableSleep) :
    serialPortExpander(serialPortExpander), disableSleep(disableSleep), numberOfValues(0) {
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
        transition(AtlasScientificBoardState::Done);
        return true;
    }
    switch (state) {
        case AtlasScientificBoardState::Start: {
            setSerial(serialPortExpander->getSerial());
            transition(AtlasScientificBoardState::DisableContinuousReading);
            break;
        }
        case AtlasScientificBoardState::DisableContinuousReading: {
            sendCommand(CMD_CONTINUOUS_OFF);
            break;
        }
        case AtlasScientificBoardState::ConfigureResponse: {
            sendCommand(CMD_RESPONSE1);
            break;
        }
        case AtlasScientificBoardState::Status0: {
            sendCommand(CMD_STATUS);
            break;
        }
        case AtlasScientificBoardState::Status1: {
            sendCommand(CMD_STATUS);
            break;
        }
        case AtlasScientificBoardState::LedsOn: {
            sendCommand(CMD_LED_ON);
            break;
        }
        case AtlasScientificBoardState::Configure: {
            sendCommand(CMD_CONTINUOUS_OFF);
            break;
        }
        case AtlasScientificBoardState::Read0: {
            sendCommand(CMD_READ);
            break;
        }
        case AtlasScientificBoardState::Read1: {
            sendCommand(CMD_READ);
            break;
        }
        case AtlasScientificBoardState::Read2: {
            sendCommand(CMD_READ);
            break;
        }
        case AtlasScientificBoardState::LedsOff: {
            sendCommand(CMD_LED_OFF);
            break;
        }
        case AtlasScientificBoardState::Sleeping: {
            sendCommand(CMD_SLEEP);
            break;
        }
        case AtlasScientificBoardState::Done: {
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

NonBlockingHandleStatus AtlasScientificBoard::handle(String reply) {
    if (reply.indexOf("*") >= 0) {
        if (reply.length() > 0) {
            DEBUG_PRINT(uint32_t(state));
            DEBUG_PRINT(">");
            DEBUG_PRINTLN(reply);
        }

        switch (state) {
            case AtlasScientificBoardState::DisableContinuousReading: {
                transition(AtlasScientificBoardState::ConfigureResponse);
                break;
            }
            case AtlasScientificBoardState::ConfigureResponse: {
                transition(AtlasScientificBoardState::Status0);
                break;
            }
            case AtlasScientificBoardState::Status0: {
                transition(AtlasScientificBoardState::Status1);
                break;
            }
            case AtlasScientificBoardState::Status1: {
                transition(AtlasScientificBoardState::LedsOn);
                break;
            }
            case AtlasScientificBoardState::LedsOn: {
                transition(AtlasScientificBoardState::Configure);
                break;
            }
            case AtlasScientificBoardState::Configure: {
                transition(AtlasScientificBoardState::Read0);
                break;
            }
            case AtlasScientificBoardState::Read0: {
                transition(AtlasScientificBoardState::Read1);
                break;
            }
            case AtlasScientificBoardState::Read1: {
                transition(AtlasScientificBoardState::Read2);
                break;
            }
            case AtlasScientificBoardState::Read2: {
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
                    if (index > position && numberOfValues < FK_ATLAS_SENSORS_PACKET_NUMBER_VALUES) {
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
                    transition(AtlasScientificBoardState::LedsOff);
                }
                else {
                    transition(AtlasScientificBoardState::Sleeping);
                }

                break;
            }
            case AtlasScientificBoardState::LedsOff: {
                transition(AtlasScientificBoardState::Done);
                break;
            }
            case AtlasScientificBoardState::Sleeping: {
                transition(AtlasScientificBoardState::Done);
                close();
                break;
            }
        }
        return NonBlockingHandleStatus::Handled;
    }

    return NonBlockingHandleStatus::Ignored;
}
