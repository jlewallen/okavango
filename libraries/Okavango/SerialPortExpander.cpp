#include "SerialPortExpander.h"

SerialPortExpander::SerialPortExpander(byte p0, byte p1) {
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

void SerialPortExpander::select(byte port) {
    digitalWrite(selector[0], bitRead(port, 0));
    digitalWrite(selector[1], bitRead(port, 1));
    delay(2); // Technically we're blocking, though... 2ms? Meh.
    this->port = port;
}
