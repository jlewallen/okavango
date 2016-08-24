#include "Platforms.h"
#include "ParallelizedAtlasScientificSensors.h"

extern const char *CMD_STATUS;
extern const char *CMD_LED_ON;
extern const char *CMD_LED_OFF;
extern const char *CMD_CONTINUOUS_OFF;
extern const char *CMD_SLEEP;
extern const char *CMD_READ;

ParallelizedAtlasScientificSensors::ParallelizedAtlasScientificSensors(SerialPortExpander *serialPortExpander, bool disableSleep) :
    serialPortExpander(serialPortExpander), portNumber(0), disableSleep(disableSleep) {
}

void ParallelizedAtlasScientificSensors::transition(ParallelizedAtlasScientificSensorsState newState) {
    state = newState;
    clearSendsCounter();
}

bool ParallelizedAtlasScientificSensors::tick() {
    if (NonBlockingSerialProtocol::tick()) {
        return true;
    }
    if (getSendsCounter() == 5) {
        transition(ParallelizedAtlasScientificSensorsState::Done);
        return true;
    }
    switch (state) {
        case ParallelizedAtlasScientificSensorsState::Start: {
            numberOfValues = 0;
            portNumber = 0;
            serialPortExpander->select(portNumber);
            transition(ParallelizedAtlasScientificSensorsState::Status0);
            break;
        }
        case ParallelizedAtlasScientificSensorsState::Status0: {
            sendCommand(CMD_STATUS);
            break;
        }
        case ParallelizedAtlasScientificSensorsState::Status1: {
            sendCommand(CMD_STATUS);
            break;
        }
        case ParallelizedAtlasScientificSensorsState::LedsOn: {
            sendCommand(CMD_LED_ON);
            break;
        }
        case ParallelizedAtlasScientificSensorsState::Configure: {
            sendCommand(CMD_CONTINUOUS_OFF);
            break;
        }
        case ParallelizedAtlasScientificSensorsState::Read0: {
            sendCommand(CMD_READ);
            break;
        }
        case ParallelizedAtlasScientificSensorsState::Read1: {
            sendCommand(CMD_READ);
            break;
        }
        case ParallelizedAtlasScientificSensorsState::Read2: {
            sendCommand(CMD_READ);
            break;
        }
        case ParallelizedAtlasScientificSensorsState::LedsOff: {
            sendCommand(CMD_LED_OFF);
            break;
        }
        case ParallelizedAtlasScientificSensorsState::Sleeping: {
            sendCommand(CMD_SLEEP);
            break;
        }
        case ParallelizedAtlasScientificSensorsState::Done: {
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

bool ParallelizedAtlasScientificSensors::handle(String reply) {
    if (reply.indexOf("*") >= 0) {
        if (reply.length() > 0) {
            DEBUG_PRINT(uint32_t(state));
            DEBUG_PRINT(">");
            DEBUG_PRINTLN(reply);
        }

        switch (state) {
            case ParallelizedAtlasScientificSensorsState::Status0: {
                transition(ParallelizedAtlasScientificSensorsState::Status1);
                break;
            }
            case ParallelizedAtlasScientificSensorsState::Status1: {
                transition(ParallelizedAtlasScientificSensorsState::LedsOn);
                break;
            }
            case ParallelizedAtlasScientificSensorsState::LedsOn: {
                transition(ParallelizedAtlasScientificSensorsState::Configure);
                break;
            }
            case ParallelizedAtlasScientificSensorsState::Configure: {
                if (portNumber < 4) {
                    portNumber++;
                    serialPortExpander->select(portNumber);
                    transition(ParallelizedAtlasScientificSensorsState::Status0);
                }
                else {
                    portNumber = 0;
                    serialPortExpander->select(portNumber);
                    transition(ParallelizedAtlasScientificSensorsState::Read0);
                }
                break;
            }
            case ParallelizedAtlasScientificSensorsState::Read0: {
                transition(ParallelizedAtlasScientificSensorsState::Read1);
                break;
            }
            case ParallelizedAtlasScientificSensorsState::Read1: {
                transition(ParallelizedAtlasScientificSensorsState::Read2);
                break;
            }
            case ParallelizedAtlasScientificSensorsState::Read2: {
                int8_t position = 0;
                numberOfValues = 0;

                String firstLine = getFirstLine(reply);

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
                    }
                    else {
                        break;
                    }
                }

                if (disableSleep) {
                    transition(ParallelizedAtlasScientificSensorsState::LedsOff);
                }
                else {
                    transition(ParallelizedAtlasScientificSensorsState::Sleeping);
                }

                break;
            }
            case ParallelizedAtlasScientificSensorsState::LedsOff: {
                transition(ParallelizedAtlasScientificSensorsState::Done);
                if (portNumber < 4) {
                    portNumber++;
                    serialPortExpander->select(portNumber);
                    transition(ParallelizedAtlasScientificSensorsState::LedsOff);
                }
                else {
                    portNumber = 0;
                    serialPortExpander->select(portNumber);
                    transition(ParallelizedAtlasScientificSensorsState::Done);
                }
                break;
            }
            case ParallelizedAtlasScientificSensorsState::Sleeping: {
                transition(ParallelizedAtlasScientificSensorsState::Done);
                if (portNumber < 4) {
                    portNumber++;
                    serialPortExpander->select(portNumber);
                    transition(ParallelizedAtlasScientificSensorsState::Sleeping);
                }
                else {
                    portNumber = 0;
                    serialPortExpander->select(portNumber);
                    transition(ParallelizedAtlasScientificSensorsState::Done);
                }
                close();
                break;
            }
        }
        return true;
    }
    return false;
}
