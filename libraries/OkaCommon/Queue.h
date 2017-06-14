#ifndef QUEUE_H
#define QUEUE_H

#include <SPI.h>
#include <SD.h>
#include "protocol.h"

class Queue {
public:
    virtual void startAtBeginning() = 0;
    virtual int16_t size() = 0;
    virtual uint8_t *dequeue() = 0;
    virtual void enqueue(uint8_t *entry, size_t size = FK_QUEUE_ENTRY_SIZE) = 0;

};

class SingleEntryQueue : public Queue {
private:
    uint8_t *saved = nullptr;
    uint8_t *entry = nullptr;

public:
    SingleEntryQueue(uint8_t *entry);

public:
    virtual void startAtBeginning() override;
    virtual int16_t size() override;
    virtual uint8_t *dequeue() override;
    virtual void enqueue(uint8_t *entry, size_t size = FK_QUEUE_ENTRY_SIZE) override;

};

class FileQueue : public Queue {
private:
    const char *filename;
    uint8_t buffer[FK_QUEUE_ENTRY_SIZE];
    uint32_t dequeuePosition;

public:
    FileQueue();
    FileQueue(const char *filename);
    int16_t size() override;
    uint8_t *dequeue() override;
    void enqueue(uint8_t *entry, size_t size = FK_QUEUE_ENTRY_SIZE) override;
    void startAtBeginning() override;
    void removeAll();
    void copyInto(Queue *into);

private:
    File open();
};

#endif
