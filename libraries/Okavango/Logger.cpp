#include "Logger.h"

Logger::Logger(uint8_t pinCs) : pinCs(pinCs) {
}

void Logger::openFile(const char *filename) {
    Serial.print("Log: ");
    Serial.println(filename);
    file = SD.open(filename, FILE_WRITE);
}

bool Logger::setup() {
    pinMode(pinCs, OUTPUT);

    if (!SD.begin(pinCs)) {
        return false;
    }

    for (uint32_t i = 0; i <= 99999999; i++) {
        char filename[13];
        String fn(i);
        while (fn.length() < 8) {
            fn = '0' + fn;
        }
        fn = fn + ".CSV";
        fn.toCharArray(filename, sizeof(filename));
        if (!SD.exists(filename)) {
            openFile(filename);
            break;
        }
        else {
            file = SD.open(filename, FILE_READ);
            uint64_t size = file.size();
            file.close();

            if (size == 0) {
                openFile(filename);
                break;
            }
        }
    }

    if (opened()) {
        file.flush();
    }
    else {
        return false;
    }

    return true;
}
