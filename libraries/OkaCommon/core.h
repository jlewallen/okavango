#ifndef CORE_H
#define CORE_H

#include <RTClib.h>

#include "Platforms.h"
#include "LoraRadio.h"
#include "Logger.h"
#include "Queue.h"

/**
 * Manages pieces that are central to any participating board.
 */
class CorePlatform {
private:
    Logger sdLogger;
    LoraRadio loraRadio;
    RTC_PCF8523 rtc;
    Queue localQueue;

public:
    CorePlatform(fk_board_t *board);

public:
    Logger *logger() { return &sdLogger; }
    LoraRadio *radio() { return &loraRadio; }
    Queue *queue() { return &localQueue; }

    uint32_t now() { return rtc.now().unixtime(); }
    void enqueue(uint8_t *data);
    uint8_t *dequeue();

public:
    void setup();
    bool tick();

};

#endif
