#ifndef NON_BLOCKING_SERIAL_H
#define NON_BLOCKING_SERIAL_H

#include <Arduino.h>
#include "Serials.h"

enum class NonBlockingSerialProtocolState {
    Reading,
    Idle,
    Closed
};

enum class NonBlockingHandleStatus {
    Handled,
    Ignored,
    Unknown
};

class NonBlockingSerialProtocol {
private:
    SerialType *serial;
    NonBlockingSerialProtocolState nbsState = NonBlockingSerialProtocolState::Idle;
    uint32_t lastStateChangeOrReplyAt;
    uint16_t replyWait;
    uint8_t sendsCounter;
    bool emptyBufferAfterEveryLine;
    bool addNewLines;
    String buffer;
    Stream *debug;

public:
    NonBlockingSerialProtocol(Stream *debug, uint16_t replyWait = 5000, bool emptyBufferAfterEveryLine = false, bool addNewLine = true);

    void drain();
    void open();

    void setSerial(SerialType *newSerial);
    SerialType *getSerial();

    virtual bool tick();

protected:
    void clearSendsCounter();
    int8_t getSendsCounter();
    void sendCommand(const char *cmd);
    void transition(NonBlockingSerialProtocolState newState);
    virtual NonBlockingHandleStatus handle(String reply);
    void appendToBuffer(char newChar);
    void close();
};

#endif
