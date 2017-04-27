#ifndef SERIAL_PORT_EXPANDER_H
#define SERIAL_PORT_EXPANDER_H

#include "Platforms.h"

/**
 * This supports up to 8, we can only choose from 4 though because we route
 * through the power isolator.
 */
class SerialPortExpander {
private:
    byte selector[2];
    SerialType *defaultSerial;
    ConductivityConfig conductivityConfig;
    byte port;

public:
    SerialPortExpander(byte p0, byte p1, ConductivityConfig conductivityConfig, SerialType *defaultSerial = nullptr);

    byte getPort() {
        return port;
    }

    SerialType *getSerial(uint32_t baud = 9600);

public:
    void setup();
    bool tick();
    void select(byte port);
};

#endif
