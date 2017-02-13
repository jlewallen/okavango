#ifndef MEMORY_H
#define MEMORY_H

#include <Arduino.h>

typedef struct fk_memory_state_t {
    char name[3];
} fk_memory_state_t;

class Memory {
private:
    bool initialized = false;
    fk_memory_state_t state;

public:
    bool isInitialized() {
        return initialized;
    }

    void setup();
    void update(String name);

    const char *getName() {
        return state.name;
    }

};

#endif
