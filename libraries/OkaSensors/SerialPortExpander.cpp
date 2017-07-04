#include "SerialPortExpander.h"

SingleSerialPortExpander::SingleSerialPortExpander(byte p0, byte p1, ConductivityConfig conductivityConfig, SerialType *defaultSerial, byte numberOfPorts) :
    conductivityConfig(conductivityConfig), defaultSerial(defaultSerial), numberOfPorts(numberOfPorts) {

    selector[0] = p0;
    selector[1] = p1;

    if (defaultSerial == nullptr) {
        defaultSerial = &Serial1;
    }
}

void SingleSerialPortExpander::setup() {
    pinMode(selector[0], OUTPUT);
    if (selector[1] > 0) {
        pinMode(selector[1], OUTPUT);
    }
    select(0);
}

SerialType *SingleSerialPortExpander::getSerial(uint32_t baud) {
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

void SingleSerialPortExpander::select(byte newPort) {
    digitalWrite(selector[0], bitRead(newPort, 0));
    if (selector[1] > 0) {
        digitalWrite(selector[1], bitRead(newPort, 1));
    }
    delay(20);
    port = newPort;
}

void DualSerialPortExpander::setup() {
    speA->setup();
    speB->setup();
}

SerialType *DualSerialPortExpander::getSerial(uint32_t baud) {
    if (this->port < 4) {
        return speA->getSerial();
    }
    return speB->getSerial();
}

void DualSerialPortExpander::select(byte newPort) {
    if (newPort < 4) {
        speA->select(newPort);
    }
    else {
        speB->select(newPort - 4);
    }
    port = newPort;
}
