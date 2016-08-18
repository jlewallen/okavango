#ifndef QUEUE_H
#define QUEUE_H

#include <SPI.h>
#include <SD.h>
#include "protocol.h"

class Queue {
private:
    const char *filename;
    uint8_t buffer[FK_QUEUE_ENTRY_SIZE];
    uint32_t dequeuePosition;

public:
    Queue();
    Queue(const char *filename);
    int16_t size();
    void removeAll();
    uint8_t *dequeue();
    void enqueue(uint8_t *buffer);
    void startAtBeginning();

private:
    File open();
};

#endif


