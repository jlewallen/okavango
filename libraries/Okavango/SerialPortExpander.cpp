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
    pinMode(selector[1], OUTPUT);
}

SerialType *SingleSerialPortExpander::getSerial(uint32_t baud) {
    if (port == 3 && conductivityConfig == OnSerial2) {
        #ifdef ARDUINO_SAMD_FEATHER_M0
        platformSerial2Begin(baud);
        return &Serial2;
        #else
        return nullptr;
        #endif
    }
    else {
        if (&Serial1 == defaultSerial) {
            Serial1.begin(baud);
        }
        else {
            #ifdef ARDUINO_SAMD_FEATHER_M0
            platformSerial2Begin(baud);
            #endif
        }
        return defaultSerial;
    }
}

void SingleSerialPortExpander::select(byte port) {
    digitalWrite(selector[0], bitRead(port, 0));
    digitalWrite(selector[1], bitRead(port, 1));
    delay(2);
    this->port = port;
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

void DualSerialPortExpander::select(byte port) {
    if (port < 4) {
        speA->select(port);
    }
    else {
        speB->select(port - 4);
    }
    this->port = port;
}
