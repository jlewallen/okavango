#ifndef NON_BLOCKING_SERIAL_H
#define NON_BLOCKING_SERIAL_H

#include "Platforms.h"

enum NonBlockingSerialProtocolState {
    Reading,
    Idle,
    Closed
};

class NonBlockingSerialProtocol {
private:
    SerialType *serial;
    NonBlockingSerialProtocolState state = Idle;
    uint32_t lastStateChangeAt;
    uint16_t replyWait;
    bool emptyBufferAfterEveryLine;
    bool addNewLines;
    String buffer;

public:
    NonBlockingSerialProtocol(uint16_t replyWait = 5000, bool emptyBufferAfterEveryLine = false, bool addNewLine = true);

    void setSerial(SerialType *newSerial) {
        serial = newSerial;
    }

    void drain() {
        uint16_t to = 0;
        while (to++ < 40) {
            while (serial->available()) {
                serial->read();
                to = 0;
            }
            delay(1);
        }
    }

    void setup();

    virtual bool tick();

    void open() {
        transition(Idle);
    }

protected:
    void sendCommand(const char *cmd);
    void transition(NonBlockingSerialProtocolState newState);
    virtual bool handle(String reply);
    void appendToBuffer(char newChar);
    void close();
};

#endif
