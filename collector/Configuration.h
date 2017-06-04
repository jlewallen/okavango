#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <Arduino.h>
#include "Memory.h"

class Configuration {
private:
    Memory *memory;
    const char *filename;
    String name;

public:
    Configuration(Memory *memory, const char *filename);

public:
    const char *getName() {
        return name.c_str();
    }

    bool read(bool sdAvailable);
    bool hasRockBlock();
};

#endif
