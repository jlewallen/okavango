#include "Queue.h"
#include "Platforms.h"

typedef struct header_t {
    bool live;
} header_t;

#define FK_QUEUE_ENTRY_SIZE_ON_DISK    (sizeof(header_t) + FK_QUEUE_ENTRY_SIZE)

Queue::Queue() : dequeuePosition(0) {
}

File Queue::open() {
    File file = SD.open(FK_SETTINGS_QUEUE_FILENAME, FILE_WRITE);
    if (!file) {
        DEBUG_PRINTLN(F("Queue unavailable"));
        return file;
    }

    return file;
}

void Queue::startAtBeginning() {
    dequeuePosition = 0;
}

int16_t Queue::size() {
    File file = open();
    if (file) {
        const uint32_t size = file.size();
        const uint32_t length = size / FK_QUEUE_ENTRY_SIZE_ON_DISK;
        file.close();
        return length;
    }

    return -1;
}

void Queue::enqueue(uint8_t *buffer) {
    File file = open();
    if (file) {
        header_t header = { true };

        file.seek(file.size());
        uint32_t written = file.write((uint8_t *)&header, sizeof(header_t));
        written += file.write(buffer, FK_QUEUE_ENTRY_SIZE);
        file.close();
    }
}

void Queue::removeAll() {
    SD.remove(FK_SETTINGS_QUEUE_FILENAME);
}

uint8_t *Queue::dequeue() {
    File file = open();
    if (file) {
        const uint32_t size = file.size();
        if (size == 0) {
            file.close();
            return NULL;
        }

        while (dequeuePosition < size) {
            // DEBUG_PRINT(F("Queue #"));
            // DEBUG_PRINTLN(dequeuePosition / QUEUE_ENTRY_SIZE);

            file.seek(dequeuePosition);

            header_t header;
            if (file.read(&header, sizeof(header_t)) != sizeof(header_t)) {
                file.close();
                DEBUG_PRINTLN("Read QH error");
                return NULL;
            }

            if (header.live) {
                if (file.read(buffer, FK_QUEUE_ENTRY_SIZE) != FK_QUEUE_ENTRY_SIZE) {
                    DEBUG_PRINTLN("Read QE error");
                    file.close();
                    return NULL;
                }

                dequeuePosition += FK_QUEUE_ENTRY_SIZE_ON_DISK;

                // DEBUG_PRINTLN(F(" returning"));
                return buffer;
            }
            else {
                // DEBUG_PRINTLN(F(" dead"));
            }
        }

        DEBUG_PRINTLN(F("End of queue"));

        file.close();

        removeAll();

        return NULL;
    }
}

