#include <Arduino.h>
#include "Tickable.h"

class SerialPortExpander {
private:
    byte selector[3];

public:
    SerialPortExpander(byte p0, byte p1, byte p2);

public:
    void setup();
    bool tick();
    void select(byte port);
};

