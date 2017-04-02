#ifndef MEMORY_H
#define MEMORY_H

#include <Arduino.h>
#include "TransmissionStatus.h"

// Turn these into number of minutes or seconds.

typedef struct fk_memory_weather_intervals_t {
    uint8_t stop;
} fk_memory_weather_intervals_t;

typedef struct fk_memory_core_intervals_t {
    uint32_t idle;
    uint32_t airwaves;
    uint32_t restart;
    fk_memory_weather_intervals_t weatherStation;
} fk_memory_core_intervals_t;

typedef struct fk_memory_state_t {
    char name[3];
    uint32_t dyingAt;
    uint32_t aliveAt;
    uint16_t restarts;
    uint32_t restartAt;
    fk_memory_core_intervals_t intervals;
    fk_transmission_schedule_t schedules[TRANSMISSION_KIND_KINDS];
} fk_memory_state_t;

uint32_t msToInterval(uint32_t ms);

uint32_t intervalToMs(uint32_t interval);

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
        return &state.intervals;
    }

    fk_transmission_schedule_t *schedules() {
        return state.schedules;
    }

};

#endif
