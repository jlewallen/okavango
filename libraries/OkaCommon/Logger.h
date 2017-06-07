#ifndef LOGGER_H
#define LOGGER_H

#include <SPI.h>
#include <SD.h>

class Logger {
public:
    static File open(const char *filename, const char *header);
    static File open(const char *filename);
};

#endif

