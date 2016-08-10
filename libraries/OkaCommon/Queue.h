#ifndef QUEUE_H
#define QUEUE_H

#include <SPI.h>
#include <SD.h>
#include "protocol.h"

class Queue {
private:
    uint8_t buffer[FK_QUEUE_ENTRY_SIZE];
    uint32_t dequeuePosition;

public:
    Queue();
    int16_t size();
    void removeAll();
    uint8_t *dequeue();
    void enqueue(uint8_t *buffer);
    void startAtBeginning();

private:
    File open();
};

#endif


