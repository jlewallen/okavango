#ifndef MEMORY_H
#define MEMORY_H

#include <Arduino.h>

typedef struct fk_memory_weather_intervals_t {
    uint32_t start;
    uint32_t ignore;
    uint32_t off;
    uint32_t reading;
} fk_memory_weather_intervals_t;

typedef struct fk_memory_core_intervals_t {
    uint32_t idle;
    uint32_t airwaves;
    uint32_t weather;
    fk_memory_weather_intervals_t weatherStation;
} fk_memory_core_intervals_t;

typedef struct fk_memory_state_t {
    char name[3];
    uint32_t dyingAt;
    uint32_t aliveAt;
    uint16_t restarts;
    fk_memory_core_intervals_t intervals;
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
    void restarting();
    void markDying(uint32_t time);
    void markAlive(uint32_t time);

    const char *getName() {
        return state.name;
    }

    fk_memory_core_intervals_t *intervals() {
        return nullptr;
    }

};

#endif
