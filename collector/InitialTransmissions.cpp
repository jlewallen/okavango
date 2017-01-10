#include "InitialTransmissions.h"
#include "Platforms.h"
#include <SD.h>

#define INITIAL_TRANSMISSIONS_FILE   "ITXDONE.INF"

typedef struct fk_initial_transmissions_t {
    uint8_t which[16];
} fk_initial_transmissions_t;

bool read(fk_initial_transmissions_t *status) {
    if (SD.exists(INITIAL_TRANSMISSIONS_FILE)) {
        File file = SD.open(INITIAL_TRANSMISSIONS_FILE, FILE_READ);
        if (!file) {
            DEBUG_PRINTLN(F("ITS: unavailable"));
            return false;
        }
        if (file.size() != sizeof(fk_initial_transmissions_t)) {
            file.close();
            DEBUG_PRINTLN(F("ITS: bad size "));
            SD.remove(INITIAL_TRANSMISSIONS_FILE);
            return false;
        }
        if (file.read((uint8_t *)status, sizeof(fk_initial_transmissions_t)) != sizeof(fk_initial_transmissions_t)) {
            file.close();
            DEBUG_PRINTLN(F("ITS: read error"));
            SD.remove(INITIAL_TRANSMISSIONS_FILE);
            return false;
        }

        file.close();

        return true;
    }

    return false;
}

bool write(fk_initial_transmissions_t *status) {
    File file = SD.open(INITIAL_TRANSMISSIONS_FILE, FILE_WRITE);
    if (!file) {
        DEBUG_PRINTLN(F("ITS: unavailable"));
        return false;
    }

    file.seek(0);

    if (file.write((uint8_t *)status, sizeof(fk_initial_transmissions_t)) != sizeof(fk_initial_transmissions_t)) {
        file.close();
        DEBUG_PRINTLN(F("ITS: write error"));
        SD.remove(INITIAL_TRANSMISSIONS_FILE);
        return false;
    }

    file.flush();
    file.close();

    return true;
}

void InitialTransmissions::markCompleted(uint8_t which) {
    fk_initial_transmissions_t initial;
    memzero((void *)&initial, sizeof(fk_initial_transmissions_t));

    read(&initial);
    initial.which[which] = true;
    write(&initial);
}

bool InitialTransmissions::alreadyDone(uint8_t which) {
    fk_initial_transmissions_t initial;
    memzero((void *)&initial, sizeof(fk_initial_transmissions_t));

    if (read(&initial)) {
        return initial.which[which];
    }
    return false;
}

bool initialWeatherTransmissionSent = false;
bool initialAtlasTransmissionSent = false;
bool initialSonarTransmissionSent = false;
bool initialLocationTransmissionSent = false;
