#include <string.h>
#include "NonBlockingSerial.h"

NonBlockingSerialProtocol::NonBlockingSerialProtocol(Stream *debug, uint16_t replyWait, bool emptyBufferAfterEveryLine, bool addNewLine) :
    debug(debug), replyWait(replyWait), emptyBufferAfterEveryLine(emptyBufferAfterEveryLine), addNewLines(addNewLine) {
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

        NonBlockingHandleStatus status = handle(buffer);
        switch (status) {
        case NonBlockingHandleStatus::Handled:
            lastStateChangeOrReplyAt = millis();
            transition(NonBlockingSerialProtocolState::Idle);
            buffer = "";
            break;
        case NonBlockingHandleStatus::Ignored:
            lastStateChangeOrReplyAt = millis();
            if (emptyBufferAfterEveryLine) {
                buffer = "";
            }
            break;
        case NonBlockingHandleStatus::Unknown:
            break;
        }
    }
}

void NonBlockingSerialProtocol::sendCommand(const char *cmd) {
    // Precondition: state == Idle
    serial->print(cmd);
    serial->print('\r');
    transition(NonBlockingSerialProtocolState::Reading);
    if (debug != nullptr) {
        debug->print("Send: ");
        debug->println(cmd);
    }
    sendsCounter++;
}

NonBlockingHandleStatus NonBlockingSerialProtocol::handle(String reply) {
    return NonBlockingHandleStatus::Handled; // No protocol here.
}

void NonBlockingSerialProtocol::transition(NonBlockingSerialProtocolState newState) {
    nbsState = newState;
    lastStateChangeOrReplyAt = millis();
}

void NonBlockingSerialProtocol::close() {
    #ifdef ARDUINO_SAMD_FEATHER_M0
    serial->end();
    #endif
    transition(NonBlockingSerialProtocolState::Closed);
}

void NonBlockingSerialProtocol::setSerial(SerialType *newSerial) {
    clearSendsCounter();
    serial = newSerial;
    flush();
}

void NonBlockingSerialProtocol::flush() {
    uint32_t started = millis();
    while (true) {
        if (millis() - started > 200) {
            break;
        }
        while (serial->available()) {
            serial->read();
        }
    }
}

SerialType *NonBlockingSerialProtocol::getSerial() {
    return serial;
}

void NonBlockingSerialProtocol::open() {
    transition(NonBlockingSerialProtocolState::Idle);
}

void NonBlockingSerialProtocol::clearSendsCounter() {
    sendsCounter = 0;
}

int8_t NonBlockingSerialProtocol::getSendsCounter() {
    return sendsCounter;
}
