#ifndef MEMORY_H
#define MEMORY_H

#include <Arduino.h>

typedef struct fk_memory_state_t {
    char name[3];
    uint32_t dyingAt;
    uint32_t aliveAt;
} fk_memory_state_t;

class Memory {
private:
    bool initialized = false;
    fk_memory_state_t state;

private:
    void save();

public:
    bool isInitialized() {
        return initialized;
    }

    void setup();
    void update(String name);
    void markDying(uint32_t time);
    void markAlive(uint32_t time);

    const char *getName() {
        return state.name;
    }

};

#endif
