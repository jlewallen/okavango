#include "Platforms.h"
#include "DoubleQueue.h"

DoubleQueue::DoubleQueue(const char *fnA, const char *fnB) :
    queueA(fnA), queueB(fnB) {

    if (queueA.size() > 0) {
        activeQueue = &queueA;
        pendingQueue = &queueB;
        DEBUG_PRINT("A Active: "); DEBUG_PRINTLN(queueA.size());
        DEBUG_PRINT("B Pending: "); DEBUG_PRINTLN(queueB.size());
    }
    else {
        activeQueue = &queueB;
        pendingQueue = &queueA;
        DEBUG_PRINT("B Active: "); DEBUG_PRINTLN(queueB.size());
        DEBUG_PRINT("A Pending: "); DEBUG_PRINTLN(queueA.size());
    }
}

void DoubleQueue::enqueue(uint8_t *entry, size_t size) {
    activeQueue->enqueue(entry, size);
}

void DoubleQueue::requeue(uint8_t *entry, size_t size) {
    pendingQueue->enqueue(entry, size);
}

uint8_t *DoubleQueue::dequeue() {
    return activeQueue->dequeue();
}
