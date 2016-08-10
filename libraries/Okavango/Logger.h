#ifndef LOGGER_H
#define LOGGER_CPP

#include <SPI.h>
#include <SD.h>

class Logger {
public:
    static File open(const char *filename);
};

#endif

