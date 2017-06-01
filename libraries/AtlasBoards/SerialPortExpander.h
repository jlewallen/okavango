#ifndef SERIAL_PORT_EXPANDER_H
#define SERIAL_PORT_EXPANDER_H

#include "NonBlockingSerial.h"
#include "Serials.h"

typedef enum ConductivityConfig {
    None,
    OnSerial2,
    OnExpanderPort4
} ConductivityConfig;

class SerialPortExpander {
public:
    virtual void setup() = 0;
    virtual byte getPort() = 0;
    virtual SerialType *getSerial(uint32_t baud = 9600) = 0;
    virtual void select(byte port) = 0;
    virtual byte getNumberOfPorts() = 0;

};

/**
 * This supports up to 8, we can only choose from 4 though because we route
 * through the power isolator.
 */
class SingleSerialPortExpander : public SerialPortExpander {
private:
    byte selector[2];
    SerialType *defaultSerial;
    ConductivityConfig conductivityConfig;
    byte port;
    byte numberOfPorts;

public:
    SingleSerialPortExpander(byte p0, byte p1, ConductivityConfig conductivityConfig, SerialType *defaultSerial = nullptr, byte numberOfPorts = 4);

public:
    void setup() override;
    SerialType *getSerial(uint32_t baud = 9600) override;
    void select(byte port) override;
    byte getPort() override {
        return port;
    }
    byte getNumberOfPorts() override {
        return numberOfPorts;
    }

};

class DualSerialPortExpander : public SerialPortExpander {
private:
    SerialPortExpander *speA;
    SerialPortExpander *speB;
    byte port;

public:
    DualSerialPortExpander(SerialPortExpander *speA, SerialPortExpander *speB) :
        speA(speA), speB(speB) {
    }

public:
    void setup() override;
    SerialType *getSerial(uint32_t baud = 9600) override;
    void select(byte port) override;
    byte getPort() override {
        return port;
    }
    byte getNumberOfPorts() override {
        return speA->getNumberOfPorts() + speB->getNumberOfPorts();
    }

};

#endif
