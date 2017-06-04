#include "ParallelizedAtlasScientificSensors.h"

const char *CMD_RESPONSE1 = "RESPONSE,1";
const char *CMD_STATUS = "STATUS";
const char *CMD_FACTORY = "FACTORY";
const char *CMD_LED_ON = "L,1";
const char *CMD_LED_OFF = "L,0";
const char *CMD_CONTINUOUS_OFF = "C,0";
const char *CMD_SLEEP = "SLEEP";
const char *CMD_READ = "R";

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

ParallelizedAtlasScientificSensors::ParallelizedAtlasScientificSensors(Stream *debug, SerialPortExpander *serialPortExpander, bool disableSleep, uint8_t maximumNumberOfRead0s) :
    NonBlockingSerialProtocol(debug), debug(debug), serialPortExpander(serialPortExpander), portNumber(0), disableSleep(disableSleep), numberOfValues(0), maximumNumberOfRead0s(maximumNumberOfRead0s) {
    for (uint8_t i = 0; i < serialPortExpander->getNumberOfPorts(); ++i) {
        hasPortFailed[i] = false;
    }
    for (uint8_t i = 0; i < FK_ATLAS_BOARD_MAXIMUM_NUMBER_VALUES; ++i) {
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
        debug->print("PORT: ");
        debug->println(portNumber);
        handle("*OK(NOT-FAILED)\r");
        return true;
    }
    else if (hasPortFailed[portNumber]) {
        debug->print("PORT: ");
        debug->println(portNumber);
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
            debug->print("PORT: ");
            debug->println(portNumber);
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
            debug->print("PORT: ");
            debug->println(portNumber);
            sendCommand(CMD_READ);
            break;
        }
        case ParallelizedAtlasScientificSensorsState::LedsOnBeforeRead: {
            sendCommand(CMD_LED_ON);
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
            debug->println("DONE");
            return false;
        }
    }
    return true;
}

NonBlockingHandleStatus ParallelizedAtlasScientificSensors::handle(String reply) {
    if (reply.indexOf("*") >= 0) {
        debug->print(uint32_t(state));
        debug->print(" ");
        debug->print(portNumber);
        debug->print(">");
        debug->println(reply);

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
            case ParallelizedAtlasScientificSensorsState::LedsOnBeforeRead: {
                portNumber++;
                if (portNumber < serialPortExpander->getNumberOfPorts()) {
                    transition(ParallelizedAtlasScientificSensorsState::LedsOnBeforeRead);
                }
                else {
                    portNumber = 0;
                    transition(ParallelizedAtlasScientificSensorsState::Read1);
                }
                serialPortExpander->select(portNumber);
                setSerial(serialPortExpander->getSerial());
                break;
            }
            case ParallelizedAtlasScientificSensorsState::Configure: {
                portNumber++;
                if (portNumber < serialPortExpander->getNumberOfPorts()) {
                    transition(ParallelizedAtlasScientificSensorsState::DisableContinuousReading);
                }
                else {
                    debug->println("Waiting...");
                    portNumber = 0;
                    transition(ParallelizedAtlasScientificSensorsState::Waiting);
                }
                serialPortExpander->select(portNumber);
                setSerial(serialPortExpander->getSerial());
                break;
            }
            case ParallelizedAtlasScientificSensorsState::Read0: {
                portNumber++;
                if (portNumber < serialPortExpander->getNumberOfPorts()) {
                    transition(ParallelizedAtlasScientificSensorsState::Read0);
                }
                else if (numberOfRead0s < maximumNumberOfRead0s) {
                    portNumber = 0;
                    numberOfRead0s++;
                    debug->print("Read0: ");
                    debug->println(numberOfRead0s);
                    transition(ParallelizedAtlasScientificSensorsState::Read0);
                }
                else {
                    portNumber = 0;
                    debug->println("Starting Real Reads");
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
                    if (index > position && numberOfValues < FK_ATLAS_BOARD_MAXIMUM_NUMBER_VALUES) {
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
                if (portNumber < serialPortExpander->getNumberOfPorts()) {
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
                    if (portNumber < serialPortExpander->getNumberOfPorts()) {
                        transition(ParallelizedAtlasScientificSensorsState::Read1);
                    }
                    else {
                        transition(ParallelizedAtlasScientificSensorsState::Done);
                    }
                    serialPortExpander->select(portNumber);
                    setSerial(serialPortExpander->getSerial());
                }
                else {
                    return NonBlockingHandleStatus::Ignored;
                }
                break;
            }
        }

        return NonBlockingHandleStatus::Handled;
    }
    else {
        Serial.print("Unknown: ");
        Serial.println(reply);
        return NonBlockingHandleStatus::Unknown;
    }

    return NonBlockingHandleStatus::Ignored;
}
