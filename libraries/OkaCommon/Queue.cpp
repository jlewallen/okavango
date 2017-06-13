#include "Queue.h"
#include "Platforms.h"

typedef struct header_t {
    bool live;
} header_t;

#define FK_QUEUE_ENTRY_SIZE_ON_DISK    (sizeof(header_t) + FK_QUEUE_ENTRY_SIZE)

FileQueue::FileQueue() : filename(FK_SETTINGS_QUEUE_FILENAME), dequeuePosition(0) {
}

FileQueue::FileQueue(const char *filename) : filename(filename), dequeuePosition(0) {
}

File FileQueue::open() {
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        DEBUG_PRINTLN(F("Queue unavailable"));
        return file;
    }

    return file;
}

void FileQueue::startAtBeginning() {
    dequeuePosition = 0;
}

int16_t FileQueue::size() {
    File file = open();
    if (file) {
        const uint32_t size = file.size();
        const uint32_t length = size / FK_QUEUE_ENTRY_SIZE_ON_DISK;
        file.close();
        return length;
    }

    return -1;
}

void FileQueue::enqueue(uint8_t *entry, size_t size) {
    File file = open();
    if (file) {
        header_t header = { true };

        memzero((uint8_t *)&buffer, FK_QUEUE_ENTRY_SIZE);
        memcpy((uint8_t *)&buffer, entry, size);

        file.seek(file.size());
        uint32_t written = file.write((uint8_t *)&header, sizeof(header_t));
        written += file.write((uint8_t *)&buffer, FK_QUEUE_ENTRY_SIZE);
        file.close();
    }
}

void FileQueue::removeAll() {
    SD.remove((char *)filename);
}

uint8_t *FileQueue::dequeue() {
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

                file.close();

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
    }

    return NULL;
}

void FileQueue::copyInto(Queue *into) {
    while (true) {
        uint8_t *data = dequeue();
        if (data == NULL) {
            return;
        }
        into->enqueue(data);
    }
}
