#include "SerialPortExpander.h"

SerialPortExpander::SerialPortExpander(byte p0, byte p1, ConductivityConfig conductivityConfig) :
    conductivityConfig(conductivityConfig) {

    selector[0] = p0;
    selector[1] = p1;
}

void SerialPortExpander::setup() {
    pinMode(selector[0], OUTPUT);
    pinMode(selector[1], OUTPUT);
}

bool SerialPortExpander::tick() {
    return false;
}

SerialType *SerialPortExpander::getSerial() {
    if (port == 3 && conductivityConfig == OnSerial2) {
        platformSerial2Begin(9600);
        return &Serial2;

    }
    else {
        Serial1.begin(9600);
        return &Serial1;
    }
}

void SerialPortExpander::select(byte port) {
    digitalWrite(selector[0], bitRead(port, 0));
    digitalWrite(selector[1], bitRead(port, 1));
    delay(2); // Technically we're blocking, though... 2ms? Meh.
    this->port = port;
}
