#include "Platforms.h"
#include "Logger.h"

File Logger::open(const char *filename) {
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        DEBUG_PRINTLN(F("Log unavailable"));
        return file;
    }
    return file;
}

