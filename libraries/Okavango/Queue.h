#ifndef QUEUE_H
#define QUEUE_CPP

#include <SPI.h>
#include <SD.h>

class Queue {
private:
    const uint8_t pinCs;
    const size_t entrySize;
    const char *filename;
    bool available;
    File file;

public:
    Queue(uint8_t pinCs, size_t entrySize, const char *filename);
    uint16_t size();
    bool setup();
    void close();
    uint8_t *dequeue();
    void enqueue(uint8_t *buffer);

};

#endif


