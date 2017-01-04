#include <string.h>
#include "NonBlockingSerial.h"

NonBlockingSerialProtocol::NonBlockingSerialProtocol(uint16_t replyWait, bool emptyBufferAfterEveryLine, bool addNewLine) :
    replyWait(replyWait), emptyBufferAfterEveryLine(emptyBufferAfterEveryLine), addNewLines(addNewLine) {
}

void NonBlockingSerialProtocol::drain() {
    uint32_t started = millis();

    while (millis() - started < 500) {
        delay(10);
        while (getSerial()->available()) {
            getSerial()->read();
        }
    }
}

bool NonBlockingSerialProtocol::tick() {
    switch (nbsState) {
    case NonBlockingSerialProtocolState::Reading: {
        if (serial->available() > 0) {
            int16_t c = serial->read();
            if (c >= 0) {
                appendToBuffer((char)c);
            }
        }
        if (millis() - lastStateChangeOrReplyAt > replyWait) {
            transition(NonBlockingSerialProtocolState::Idle);
            buffer = "";
        }
        return true;
    }
    case NonBlockingSerialProtocolState::Idle: {
        break;
    }
    }

    return false;
}

void NonBlockingSerialProtocol::appendToBuffer(char newChar) {
    /* Most of our protocols work like this, so let's start here. */
    buffer += newChar;
    if (newChar == '\r') {
        if (addNewLines) {
            buffer += '\n';
        }

        lastStateChangeOrReplyAt = millis();

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
    DEBUG_PRINTLN(cmd);
    sendsCounter++;
}

bool NonBlockingSerialProtocol::handle(String reply) {
    return true; // No protocol here.
}

void NonBlockingSerialProtocol::transition(NonBlockingSerialProtocolState newState) {
    nbsState = newState;
    lastStateChangeOrReplyAt = millis();
}

void NonBlockingSerialProtocol::close() {
    serial->end();
    transition(NonBlockingSerialProtocolState::Closed);
}
