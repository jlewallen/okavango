#include <String.h>
#include "NonBlockingSerial.h"

NonBlockingSerialProtocol::NonBlockingSerialProtocol(bool emptyBufferAfterEveryLine) :  emptyBufferAfterEveryLine(emptyBufferAfterEveryLine) {
}

void NonBlockingSerialProtocol::setup() {
    serial->begin(9600);
}

bool NonBlockingSerialProtocol::tick() {
    switch (state) {
    case Reading: {
        if (serial->available() > 0) {
            int16_t c = serial->read();
            if (c >= 0) {
                appendToBuffer((char)c);
            }
        }
        if (millis() - lastStateChangeAt > 5000) {
            transition(NonBlockingSerialProtocolState::Idle);
            buffer = "";
        }
        return true;
    }
    case Idle: {
        break;
    }
    }

    return false;
}

void NonBlockingSerialProtocol::appendToBuffer(char newChar) {
    /* Most of our protocols work like this, so let's start here. */
    buffer += (char)newChar;
    if (newChar == '\r') {
        // buffer += '\n';

        if (handle(buffer)) {
            transition(NonBlockingSerialProtocolState::Idle);
            buffer = "";
        }
        else if (emptyBufferAfterEveryLine) {
            buffer = "";
        }
    }
}

void NonBlockingSerialProtocol::sendCommand(const char *cmd) {
    // Precondition: state == Idle
    serial->print(cmd);  
    serial->print('\r');
    transition(NonBlockingSerialProtocolState::Reading);
    Serial.println(cmd);
}

bool NonBlockingSerialProtocol::handle(String reply) {
    return true; // No protocol here.
}

void NonBlockingSerialProtocol::transition(NonBlockingSerialProtocolState newState) {
    state = newState;
    lastStateChangeAt = millis();
}

void NonBlockingSerialProtocol::close() {
    serial->end();
    transition(NonBlockingSerialProtocolState::Closed);
}
