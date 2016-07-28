#ifndef NON_BLOCKING_SERIAL_H
#define NON_BLOCKING_SERIAL_H

#include <SoftwareSerial.h>
#include <Arduino.h>
#include "Tickable.h"

enum NonBlockingSerialProtocolState {
    Reading,
    Idle,
    Closed
};

class NonBlockingSerialProtocol : public Tickable {
private:
    SoftwareSerial *serial;
    NonBlockingSerialProtocolState state = Idle;
    uint32_t lastStateChangeAt;
    String buffer;

public:
    NonBlockingSerialProtocol();

    void setSerial(SoftwareSerial *newSerial) {
        serial = newSerial;
    }

    void setup();

    virtual bool tick();

    void open() {
        transition(Idle);
    }

protected:
    void sendCommand(const char *cmd, bool expectReply = true);
    void transition(NonBlockingSerialProtocolState newState);
    virtual bool handle(String reply);
    void appendToBuffer(char newChar);
    void close();
};

#endif
