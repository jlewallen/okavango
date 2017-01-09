#include <SD.h>
#include "LogPrinter.h"

bool fileAvailable = false;
File fileLog;
LogPrinter logPrinter;

bool LogPrinter::open() {
    fileLog = SD.open("DEBUG.LOG", FILE_WRITE);
    if (fileLog) {
        fileAvailable = true;
    }
    else {
        fileAvailable = false;
    }
    // platformSerial3Begin(115200);
    return fileAvailable;
}

void LogPrinter::flush() {
    if (fileAvailable) {
        fileLog.flush();
    }
    Serial.flush();
}

int LogPrinter::available() {
    return 0;
}

int LogPrinter::read() {
    return -1;
}

int LogPrinter::peek() {
    return -1;
}

size_t LogPrinter::write(uint8_t c) {
    if (fileAvailable) {
        size_t w = fileLog.write(c);
        Serial.write(c);
        return w;
    }
    // Serial3.write(c);
    return Serial.write(c);
}

size_t LogPrinter::write(const uint8_t *buffer, size_t size) {
    if (fileAvailable) {
        size_t w = fileLog.write(buffer, size);
        Serial.write(buffer, size);
        return w;
    }
    // Serial3.write(buffer, size);
    return Serial.write(buffer, size);
}
