#include <String.h>
#include "AtlasScientific.h"

const char *CMD_STATUS = "STATUS";
const char *CMD_LED_ON = "L,1";
const char *CMD_LED_OFF = "L,0";
const char *CMD_CONTINUOUS_OFF = "C,0";
const char *CMD_SLEEP = "SLEEP";
const char *CMD_READ = "R";

AtlasScientificBoard::AtlasScientificBoard(byte rx, byte tx) : NonBlockingSerialProtocol(rx, tx) {
}

bool AtlasScientificBoard::tick() {
    if (NonBlockingSerialProtocol::tick()) {
        return true;
    }
    switch (state) {
        case Start: {
            Serial.println("->Status0");
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

uint8_t numberOfOccurances(String &str, char chr) {
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
        return numberOfOccurances(buffer, '\r') == 2;
    }

    return NonBlockingSerialProtocol::areWeDoneReading(buffer, newChar);
}

void AtlasScientificBoard::handle(String reply) {
    Serial.println(reply);
    switch (state) {
        case Status0: {
            Serial.println("->Status1");
            transition(Status1);
            break;
        }
        case Status1: {
            Serial.println("->Leds");
            transition(Leds);
            break;
        }
        case Leds: {
            Serial.println("->Configure");
            transition(Configure);
            break;
        }
        case Configure: {
            Serial.println("->Read0");
            transition(Read0);
            break;
        }
        case Read0: {
            Serial.println("->Read1");
            transition(Read1);
            break;
        }
        case Read1: {
            Serial.println("->Read2");
            transition(Read2);
            break;
        }
        case Read2: {
            Serial.println("->Sleeping");
            transition(Sleeping);
            break;
        }
    }
}

PhBoard::PhBoard(byte rx, byte tx) : AtlasScientificBoard(rx, tx) {
}

