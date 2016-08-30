#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <Arduino.h>

class Configuration {
private:
    const char *filename;
    String name;
    bool hasFona;
    bool hasRockBlock;
    bool initialTransmissions;

public:
    Configuration(const char *filename);

public:
    const char *getName() {
        return name.c_str();
    }

    bool hasRockBlockAttached() {
        return hasRockBlock;
    }

    bool hasFonaAttached() {
        return hasFona;
    }

    bool sendInitialTransmissions() {
        return initialTransmissions;
    }

    bool read();
};

#endif
