#include "Queue.h"

typedef struct header_t {
    bool live;
} header_t;

Queue::Queue(uint8_t pinCs, size_t entrySize, const char *filename) :
    pinCs(pinCs), entrySize(entrySize), filename(filename) {
}

bool Queue::setup() {
    pinMode(pinCs, OUTPUT);

    if (!SD.begin(pinCs)) {
        available = false;
    }

    file = SD.open(filename, FILE_WRITE);
    file.seek(0);

    return available;
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
        header_t header = {
            true
        };

        // TODO: Find free position.
        file.seek(file.size());
        file.write((uint8_t *)&header, sizeof(header_t));
        file.write(buffer, entrySize);
        file.flush();
    }
}

uint8_t *Queue::dequeue() {
    if (!available) {
        return NULL;
    }

    const size_t interval = sizeof(header_t) + entrySize;
    const uint32_t size = file.size();
    uint32_t position = file.position();

    while (position < size) {
        Serial.print("Queue #");
        Serial.print(position / interval);

        header_t header;
        if (file.read(&header, sizeof(header_t)) != sizeof(header_t)) {
            return NULL;
        }

        if (header.live) {
            uint8_t *buffer = (uint8_t *)malloc(entrySize);
            if (file.read(buffer, entrySize) != entrySize) {
                free(buffer);
                return NULL;
            }

            Serial.println(" returning");
            return buffer;
        }
        else {
            Serial.println(" dead");
        }

        position += interval;
    }

    Serial.println("End of queue");

    file.close();

    SD.remove(filename);

    file = SD.open(filename, FILE_WRITE);

    return NULL;
}

