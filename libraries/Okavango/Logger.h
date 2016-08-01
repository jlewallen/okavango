#ifndef LOGGER_H
#define LOGGER_CPP

#include <SPI.h>
#include <SD.h>

class Logger {
private:
    const uint8_t pinCs;
    File file;

private:
    void openFile(const char *filename);

public:
    Logger(uint8_t pinCs);
    bool setup();

    bool opened() {
        return file;
    }

    File &log() {
        return file;
    }
};

#endif

