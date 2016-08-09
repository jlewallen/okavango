#ifndef QUEUE_H
#define QUEUE_CPP

#include <SPI.h>
#include <SD.h>
#include "protocol.h"

class Queue {
private:
    uint8_t buffer[FK_QUEUE_ENTRY_SIZE];
    const size_t entrySize;
    const char *filename;
    bool available;
    File file;

public:
    Queue(size_t entrySize, const char *filename);
    void startAtBeginning();
    uint16_t size();
    bool setup();
    void close();
    void removeAll();
    uint8_t *dequeue();
    void enqueue(uint8_t *buffer);

};

#endif


