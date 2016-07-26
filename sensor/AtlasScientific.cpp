#include <String.h>
#include "AtlasScientific.h"

const char *CMD_STATUS = "STATUS";
const char *CMD_LED_ON = "L,1";
const char *CMD_LED_OFF = "L,0";
const char *CMD_CONTINUOUS_OFF = "C,0";
const char *CMD_SLEEP = "SLEEP";
const char *CMD_READ = "R";

AtlasScientificBoard::AtlasScientificBoard(byte rx, byte tx) : serial(rx, tx) {
}

void AtlasScientificBoard::setup() {
    serial.begin(9600);
}

String AtlasScientificBoard::buffer(long duration) {
    String raw = "";
    uint32_t started = millis();
    while (millis() - started < duration) {
        if (serial.available() > 0) {
            int32_t c = serial.read();
            if (c >= 0) {
                raw += (char)c;
                if (c == '\r') {
                    raw += '\n';
                    return raw;
                }
            }
        }
        delay(50);
    }
    return "";
}

String AtlasScientificBoard::sendCommand(const char *cmd, bool ignoreReply) {
    serial.print(cmd);  
    serial.print("\r");  
    if (ignoreReply) {
        #if ATLAS_SCIENTIFIC_ECHO
        Serial.println(cmd);
        #endif
        return "";
    }
    String reply = buffer(ATLAS_SCIENTIFIC_DELAY);

    #if ATLAS_SCIENTIFIC_ECHO
    Serial.print(cmd);
    Serial.print(": ");
    if (reply.endsWith("\n")) {
        Serial.print(reply);
    }
    else {
        Serial.println(reply);
    }
    #endif
    return reply;
}

void AtlasScientificBoard::wakeup() {
    for (byte i = 0; i < 2; ++i) {
        String reply = sendCommand(CMD_STATUS);
        if (reply.indexOf(CMD_STATUS) >= 0) {
            return;
        }
        delay(ATLAS_SCIENTIFIC_DELAY);
    }
    // TODO: HARD RESET
}

float AtlasScientificBoard::sample() {
    wakeup();

    #if ATLAS_SCIENTIFIC_LEDS
    leds(true);
    #else
    leds(false);
    #endif

    sendCommand(CMD_CONTINUOUS_OFF);

    String raw;
    for (byte i = 0; i < ATLAS_SCIENTIFIC_NUMBER_OF_READINGS; ++i) {
        sendCommand(CMD_READ);
        raw = buffer(ATLAS_SCIENTIFIC_DELAY);
    }

    serial.end();

    if (raw.length() > 0) {
        Serial.println(raw);
        return raw.toFloat();
    }

    return 9999.0f;
}

void AtlasScientificBoard::leds(bool enabled) {
    sendCommand(enabled ? CMD_LED_ON : CMD_LED_OFF);
}

void AtlasScientificBoard::sleep() {
    #if ATLAS_SCIENTIFIC_SLEEP          
    sendCommand(CMD_SLEEP, true);
    #endif
}

PhBoard::PhBoard(byte rx, byte tx) : AtlasScientificBoard(rx, tx) {
}

DissolvedOxygenBoard::DissolvedOxygenBoard(byte rx, byte tx) : AtlasScientificBoard(rx, tx) {
}

OrpBoard::OrpBoard(byte rx, byte tx) : AtlasScientificBoard(rx, tx) {
}

ConductivityBoard::ConductivityBoard(byte rx, byte tx) : AtlasScientificBoard(rx, tx) {
}
