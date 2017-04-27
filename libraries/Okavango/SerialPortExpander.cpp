#include "SerialPortExpander.h"

SerialPortExpander::SerialPortExpander(byte p0, byte p1, ConductivityConfig conductivityConfig, SerialType *defaultSerial) :
    conductivityConfig(conductivityConfig), defaultSerial(defaultSerial) {

    selector[0] = p0;
    selector[1] = p1;

    if (defaultSerial == nullptr) {
        defaultSerial = &Serial1;
    }
}

void SerialPortExpander::setup() {
    pinMode(selector[0], OUTPUT);
    pinMode(selector[1], OUTPUT);
}

bool SerialPortExpander::tick() {
    return false;
}

SerialType *SerialPortExpander::getSerial(uint32_t baud) {
    if (port == 3 && conductivityConfig == OnSerial2) {
        platformSerial2Begin(baud);
        return &Serial2;
    }
    else {
        if (&Serial1 == defaultSerial) {
            Serial1.begin(baud);
        }
        else {
            platformSerial2Begin(baud);
        }
        return defaultSerial;
    }
}

void SerialPortExpander::select(byte port) {
    digitalWrite(selector[0], bitRead(port, 0));
    digitalWrite(selector[1], bitRead(port, 1));
    delay(2); // Technically we're blocking, though... 2ms? Meh.
    this->port = port;
}
