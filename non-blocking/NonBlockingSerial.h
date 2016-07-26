#include <SoftwareSerial.h>
#include <Arduino.h>

enum NonBlockingSerialProtocolState {
    Reading,
    Idle,
    Closed
};

class NonBlockingSerialProtocol {
private:
    SoftwareSerial serial;
    NonBlockingSerialProtocolState state = Idle;
    uint32_t lastStateChangeAt = 0;
    String buffer;

public:
    NonBlockingSerialProtocol(byte rx, byte tx);

    void setup();

    virtual bool tick();

protected:
    void sendCommand(const char *cmd, bool expectReply = true);
    void transition(NonBlockingSerialProtocolState newState);
    virtual void handle(String reply);
    virtual bool areWeDoneReading(String &buffer, char newChar);
    void appendToBuffer(char newChar);
    void close();
};

