#include "Platforms.h"
#include "ParallelizedAtlasScientificSensors.h"

extern const char *CMD_RESPONSE1;
extern const char *CMD_STATUS;
extern const char *CMD_LED_ON;
extern const char *CMD_LED_OFF;
extern const char *CMD_CONTINUOUS_OFF;
extern const char *CMD_SLEEP;
extern const char *CMD_READ;

extern uint8_t numberOfOccurences(String &str, char chr);

extern String getFirstLine(String &str);

ParallelizedAtlasScientificSensors::ParallelizedAtlasScientificSensors(SerialPortExpander *serialPortExpander, bool disableSleep) :
    serialPortExpander(serialPortExpander), portNumber(0), disableSleep(disableSleep), numberOfValues(0) {
    for (uint8_t i = 0; i < 4; ++i) {
        hasPortFailed[i] = false;
    }
    for (uint8_t i = 0; i < FK_ATLAS_SENSORS_PACKET_NUMBER_VALUES; ++i) {
        values[i] = 0.0;
    }
}

void ParallelizedAtlasScientificSensors::transition(ParallelizedAtlasScientificSensorsState newState) {
    state = newState;
    lastTransisitonAt = millis();
    clearSendsCounter();
}

bool ParallelizedAtlasScientificSensors::tick() {
    if (NonBlockingSerialProtocol::tick()) {
        return true;
    }
    if (getSendsCounter() == 5) {
        hasPortFailed[portNumber] = true;
        handle("*OK(NOT)\r");
        return true;
    }
    else if (hasPortFailed[portNumber]) {
        if (state == ParallelizedAtlasScientificSensorsState::Sleeping) {
            handle("*SL(NOT)\r");
        }
        else {
            handle("*OK(NOT)\r");
        }
        return true;
    }
    switch (state) {
        case ParallelizedAtlasScientificSensorsState::Start: {
            portNumber = 0;
            serialPortExpander->select(portNumber);
            setSerial(serialPortExpander->getSerial());
            transition(ParallelizedAtlasScientificSensorsState::DisableContinuousReading);
            break;
        }
        case ParallelizedAtlasScientificSensorsState::DisableContinuousReading: {
            DEBUG_PRINT("PORT: ");
            DEBUG_PRINTLN(portNumber);
            sendCommand(CMD_CONTINUOUS_OFF);
            break;
        }
        case ParallelizedAtlasScientificSensorsState::ConfigureResponse: {
            sendCommand(CMD_RESPONSE1);
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
        case ParallelizedAtlasScientificSensorsState::Waiting: {
            if (millis() - lastTransisitonAt > 5000) {
                numberOfRead0s = 0;
                transition(ParallelizedAtlasScientificSensorsState::Read0);
            }
            break;
        }
        case ParallelizedAtlasScientificSensorsState::Read0: {
            DEBUG_PRINT("PORT: ");
            DEBUG_PRINTLN(portNumber);
            sendCommand(CMD_READ);
            break;
        }
        case ParallelizedAtlasScientificSensorsState::Read1: {
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

bool ParallelizedAtlasScientificSensors::handle(String reply) {
    if (reply.indexOf("*") >= 0) {
        if (reply.length() > 0) {
            DEBUG_PRINT(uint32_t(state));
            DEBUG_PRINT(">");
            DEBUG_PRINTLN(reply);
        }

        switch (state) {
            case ParallelizedAtlasScientificSensorsState::DisableContinuousReading: {
                transition(ParallelizedAtlasScientificSensorsState::ConfigureResponse);
                break;
            }
            case ParallelizedAtlasScientificSensorsState::ConfigureResponse: {
                transition(ParallelizedAtlasScientificSensorsState::Status0);
                break;
            }
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
                portNumber++;
                if (portNumber < 4) {
                    transition(ParallelizedAtlasScientificSensorsState::Status0);
                }
                else {
                    DEBUG_PRINTLN("Waiting...");
                    portNumber = 0;
                    transition(ParallelizedAtlasScientificSensorsState::Waiting);
                }
                serialPortExpander->select(portNumber);
                setSerial(serialPortExpander->getSerial());
                break;
            }
            case ParallelizedAtlasScientificSensorsState::Read0: {
                portNumber++;
                if (portNumber < 4) {
                    transition(ParallelizedAtlasScientificSensorsState::Read0);
                }
                else if (numberOfRead0s < 16) {
                    portNumber = 0;
                    numberOfRead0s++;
                    DEBUG_PRINT("Read0: ");
                    DEBUG_PRINTLN(numberOfRead0s);
                    transition(ParallelizedAtlasScientificSensorsState::Read0);
                }
                else {
                    portNumber = 0;
                    DEBUG_PRINTLN("Starting Real Reads");
                    transition(ParallelizedAtlasScientificSensorsState::Read1);
                }
                serialPortExpander->select(portNumber);
                setSerial(serialPortExpander->getSerial());
                break;
            }
            case ParallelizedAtlasScientificSensorsState::Read1: {
                int8_t position = 0;

                String firstLine = getFirstLine(reply);

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
                portNumber++;
                if (portNumber < 4) {
                    transition(ParallelizedAtlasScientificSensorsState::Read1);
                }
                else {
                    transition(ParallelizedAtlasScientificSensorsState::Done);
                }
                serialPortExpander->select(portNumber);
                setSerial(serialPortExpander->getSerial());
                break;
            }
            case ParallelizedAtlasScientificSensorsState::Sleeping: {
                if (reply.indexOf("*SL") >= 0) {
                    portNumber++;
                    if (portNumber < 4) {
                        transition(ParallelizedAtlasScientificSensorsState::Read1);
                    }
                    else {
                        transition(ParallelizedAtlasScientificSensorsState::Done);
                    }
                    serialPortExpander->select(portNumber);
                    setSerial(serialPortExpander->getSerial());
                }
                else {
                    return false;
                }
                break;
            }
        }
        return true;
    }
    return false;
}
