#include "Platforms.h"
#include "Logger.h"

File Logger::open(const char *filename, const char *header) {
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        DEBUG_PRINTLN(F("Log unavailable"));
        return file;
    }
    if (file.position() == 0) {
        file.println(header);
    }
    return file;
}


File Logger::open(const char *filename) {
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        DEBUG_PRINTLN(F("Log unavailable"));
        return file;
    }
    return file;
}

