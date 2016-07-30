#include "Platforms.h"
#include "Tickable.h"

class SerialPortExpander {
private:
    byte selector[3];
    byte port;

public:
    SerialPortExpander(byte p0, byte p1, byte p2);

    byte getPort() {
        return port;
    }

public:
    void setup();
    bool tick();
    void select(byte port);
};

