#include <String.h>
#include "Platforms.h"
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
            break;
        }
        case Done: {
            Serial.println("DONE");
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

bool AtlasScientificBoard::handle(String reply) {
    if (reply.indexOf("*") >= 0) {
        Serial.println(reply);

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
                int8_t position = 0;
                numberOfValues = 0;
                while (true) {
                    int16_t index = reply.indexOf(',', position);
                    if (index < 0) {
                        index = reply.indexOf('\r', position);
                    }
                    if (index < 0) {
                        index = reply.indexOf('\n', position);
                    }
                    if (index > position && numberOfValues < MAX_VALUES) {
                        String part = reply.substring(position, index);
                        values[numberOfValues++] = part.toFloat();
                        position = index + 1;
                    }
                    else {
                        break;
                    }
                }

                transition(Sleeping);

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

