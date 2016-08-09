#include "Platforms.h"
#include "Logger.h"

Logger::Logger() {
}

void Logger::openFile(const char *filename) {
    file = SD.open(filename, FILE_WRITE);
    if (!file) {
        DEBUG_PRINTLN(F("Log unavailable"));
    }
}

bool Logger::setup() {
    openFile("data.csv");

    if (opened()) {
        file.flush();
    }
    else {
        return false;
    }

    return true;
}
