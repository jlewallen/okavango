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
    String buffer;

public:
    NonBlockingSerialProtocol();

    void setSerial(SerialType *newSerial) {
        serial = newSerial;
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
