#include <SD.h>
#include "LogPrinter.h"
#include "Platforms.h"

bool fileAvailable = false;
File fileLog;
LogPrinter logPrinter;

bool LogPrinter::open(bool serial1Relay, bool serial2Relay) {
    this->serial1Relay = serial1Relay;
    this->serial2Relay = serial2Relay;

    Serial.print("Serial1Relay: ");
    Serial.println(this->serial1Relay);

    Serial.print("Serial2Relay: ");
    Serial.println(this->serial2Relay);

    fileLog = SD.open("DEBUG.LOG", FILE_WRITE);
    if (fileLog) {
        fileAvailable = true;
    }
    else {
        fileAvailable = false;
    }
    return fileAvailable;
}

void LogPrinter::flush() {
    if (serial1Relay) {
        Serial1.flush();
    }
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
    if (serial1Relay) {
        Serial1.write(c);
    }
    if (fileAvailable) {
        fileLog.write(c);
    }
    return Serial.write(c);
}

size_t LogPrinter::write(const uint8_t *buffer, size_t size) {
    if (serial1Relay) {
        Serial1.write(buffer, size);
    }
    if (fileAvailable) {
        fileLog.write(buffer, size);
    }
    return Serial.write(buffer, size);
}
