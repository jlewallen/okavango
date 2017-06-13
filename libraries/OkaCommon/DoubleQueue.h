#ifndef DOUBLE_QUEUE_H_INCLUDED
#define DOUBLE_QUEUE_H_INCLUDED

#include "Queue.h"

class DoubleQueue {
private:
    FileQueue queueA;
    FileQueue queueB;
    Queue *activeQueue;
    Queue *pendingQueue;

public:
    DoubleQueue(const char *fnA, const char *fnB);

public:
    void enqueue(uint8_t *entry, size_t size);
    void requeue(uint8_t *entry, size_t size);
    uint8_t *dequeue();

};

#endif
