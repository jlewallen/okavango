#include "Queue.h"
#include "Platforms.h"

typedef struct header_t {
    bool live;
} header_t;

Queue::Queue(size_t entrySize, const char *filename) :
    entrySize(entrySize), filename(filename) {
}

bool Queue::setup() {
    file = SD.open(filename, FILE_WRITE);
    if (!file) {
        DEBUG_PRINTLN(F("Queue unavailable"));
        return false;
    }

    // DEBUG_PRINT(F("Queue size: "));
    // DEBUG_PRINTLN(size());

    startAtBeginning();
    available = true;

    return available;
}

void Queue::startAtBeginning() {
    file.seek(0);
}

void Queue::close() {
    file.close();
}

uint16_t Queue::size() {
    if (!available) {
        return 0;
    }

    const uint32_t size = file.size();

    return size / (sizeof(header_t) + entrySize);
}

void Queue::enqueue(uint8_t *buffer) {
    if (available) {
        header_t header = { true };

        // TODO: Find free position.
        file.seek(file.size());
        uint32_t written = file.write((uint8_t *)&header, sizeof(header_t));
        written += file.write(buffer, entrySize);
        file.flush();
    }
}

void Queue::removeAll() {
    if (file) {
        file.close();
    }
    if (!SD.remove(filename)) {
        Serial.println("Error");
    }
    file = SD.open(filename, FILE_WRITE);
}

uint8_t *Queue::dequeue() {
    if (!available) {
        return NULL;
    }

    const uint32_t size = file.size();
    if (size == 0) {
        return NULL;
    }

    const size_t interval = sizeof(header_t) + entrySize;
    uint32_t position = file.position();

    while (position < size) {
        // DEBUG_PRINT(F("Queue #"));
        // DEBUG_PRINTLN(position / interval);

        header_t header;
        if (file.read(&header, sizeof(header_t)) != sizeof(header_t)) {
            return NULL;
        }

        if (header.live) {
            if (file.read(buffer, entrySize) != entrySize) {
                return NULL;
            }

            // DEBUG_PRINTLN(F(" returning"));
            return buffer;
        }
        else {
            // DEBUG_PRINTLN(F(" dead"));
        }

        position += interval;
    }

    DEBUG_PRINTLN(F("End of queue"));

    removeAll();

    return NULL;
}

