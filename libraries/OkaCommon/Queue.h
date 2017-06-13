#ifndef QUEUE_H
#define QUEUE_H

#include <SPI.h>
#include <SD.h>
#include "protocol.h"

class IQueue {
public:
    virtual void startAtBeginning() = 0;
    virtual int16_t size() = 0;
    virtual uint8_t *dequeue() = 0;
    virtual void enqueue(uint8_t *entry, size_t size = FK_QUEUE_ENTRY_SIZE) = 0;

};

class Queue : public IQueue {
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
    void enqueue(uint8_t *entry, size_t size = FK_QUEUE_ENTRY_SIZE);
    void startAtBeginning();
    void copyInto(Queue *into);

private:
    File open();
};

#endif
