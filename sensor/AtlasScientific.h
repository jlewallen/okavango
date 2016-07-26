#include <SoftwareSerial.h>
#include <Arduino.h>
#include "config.h"

class AtlasScientificBoard {
private:
    SoftwareSerial serial;

public:
    AtlasScientificBoard(byte rx, byte tx);

    void wakeup();
    void setup();
    float sample();
    void sleep();
    void leds(bool enabled);

private:
    String buffer(long duration);
    String sendCommand(const char *cmd, bool ignoreReply = false);
};

class PhBoard : public AtlasScientificBoard {
public:
    PhBoard(byte rx, byte tx);
};

class DissolvedOxygenBoard : public AtlasScientificBoard {
public:
    DissolvedOxygenBoard(byte rx, byte tx);
};

class OrpBoard : public AtlasScientificBoard {
public:
    OrpBoard(byte rx, byte tx);
};

class ConductivityBoard : public AtlasScientificBoard {
public:
    ConductivityBoard(byte rx, byte tx);
};
