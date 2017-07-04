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
    if (debug != nullptr) {
        debug->print("Send: ");
        debug->println(cmd);
    }
    serial->print(cmd);
    serial->print('\r');
    transition(NonBlockingSerialProtocolState::Reading);
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
    delay(10);
    flush();
}

void NonBlockingSerialProtocol::flush() {
    uint32_t started = millis();
    uint32_t read = 0;

    while (true) {
        if (millis() - started > 50) {
            break;
        }
        while (serial->available()) {
            if (read == 0) {
                Serial.print("FLUSH: ");
            }
            char c = serial->read();
            Serial.print(c);
            read++;
        }
    }
    if (read > 0) {
        Serial.println("");
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
