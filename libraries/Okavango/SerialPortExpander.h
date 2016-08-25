#ifndef SERIAL_PORT_EXPANDER_H
#define SERIAL_PORT_EXPANDER_H

#include "Platforms.h"

/**
 * This supports up to 8, we can only choose from 4 though because we route
 * through the power isolator.
 */
class SerialPortExpander {
private:
    ConductivityConfig conductivityConfig;
    byte selector[2];
    byte port;

public:
    SerialPortExpander(byte p0, byte p1, ConductivityConfig conductivityConfig);

    byte getPort() {
        return port;
    }

    SerialType *getSerial() {
        if (port == 3 && conductivityConfig == OnSerial2) {
            return &Serial2;

        }
        else {
            Serial1.begin(9600);
            return &Serial1;
        }
    }

public:
    void setup();
    bool tick();
    void select(byte port);
};

#endif
