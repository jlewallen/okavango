#include "Platforms.h"
#include "Tickable.h"

/**
 * This supports up to 8, we can only choose from 4 though because we route
 * through the power isolator.
 */
class SerialPortExpander {
private:
    byte selector[2];
    byte port;

public:
    SerialPortExpander(byte p0, byte p1);

    byte getPort() {
        return port;
    }

public:
    void setup();
    bool tick();
    void select(byte port);
};

