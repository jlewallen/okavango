#include <SD.h>
#include "Platforms.h"
#include "AuthToken.h"

bool AuthToken::read(const char *fn) {
    size = 0;

    if (SD.exists(fn)) {
        File file = SD.open(fn);
        if (file) {
            size = file.read(buffer, sizeof(buffer));

            file.close();
        }
    }

    return size > 0;
}

size_t AuthToken::include(uint8_t *ptr, size_t available) {
    if (size == 0) {
        DEBUG_PRINTLN("No AuthToken.");
        return 0;
    }
    if (available < size) {
        DEBUG_PRINTLN("Buffer too small to include AuthToken!");
        return 0;
    }
    if (size > 0) {
        memcpy(ptr, buffer, size);
    }

    return size;
}
