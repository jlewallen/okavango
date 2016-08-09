#ifndef LOGGER_H
#define LOGGER_CPP

#include <SPI.h>
#include <SD.h>

class Logger {
private:
    File file;

private:
    void openFile(const char *filename);

public:
    Logger();
    bool setup();

    bool opened() {
        return file;
    }

    File &log() {
        return file;
    }
};

#endif

