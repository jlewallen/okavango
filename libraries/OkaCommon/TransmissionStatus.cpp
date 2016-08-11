#include <SD.h>

#include "Platforms.h"
#include "TransmissionStatus.h"
#include "core.h"

typedef struct fk_transmission_status_t {
    uint32_t time;
    uint32_t millis;
    uint32_t elapsed;
} fk_transmission_status_t;

bool read(fk_transmission_status_t *status) {
    if (SD.exists(FK_SETTINGS_TRANSMISSION_STATUS_FILENAME)) {
        File file = SD.open(FK_SETTINGS_TRANSMISSION_STATUS_FILENAME, FILE_READ);
        if (!file) {
            DEBUG_PRINTLN(F("TS: unavailable"));
            return false;
        }
        if (file.size() != sizeof(fk_transmission_status_t)) {
            file.close();
            DEBUG_PRINTLN(F("TS: bad size "));
            SD.remove(FK_SETTINGS_TRANSMISSION_STATUS_FILENAME);
            return false;
        }
        if (file.read((uint8_t *)status, sizeof(fk_transmission_status_t)) != sizeof(fk_transmission_status_t)) {
            file.close();
            DEBUG_PRINTLN(F("TS: read error"));
            SD.remove(FK_SETTINGS_TRANSMISSION_STATUS_FILENAME);
            return false;
        }

        return true;
    }
}

bool write(fk_transmission_status_t *status) {
    File file = SD.open(FK_SETTINGS_TRANSMISSION_STATUS_FILENAME, FILE_WRITE);
    if (!file) {
        DEBUG_PRINTLN(F("TS: unavailable"));
        return false;
    }

    file.seek(0);

    if (file.write((uint8_t *)&status, sizeof(fk_transmission_status_t)) != sizeof(fk_transmission_status_t)) {
        file.close();
        DEBUG_PRINTLN(F("TS: write error"));
        SD.remove(FK_SETTINGS_TRANSMISSION_STATUS_FILENAME);
        return false;
    }

    file.flush();
    file.close();

    return true;
}

bool TransmissionStatus::shouldWe() {
    fk_transmission_status_t status;

    if (!read(&status)) {
        status.time = SystemClock.now();
        status.millis = millis();
        status.elapsed = 0;
    }

    status.elapsed += millis() - status.millis;
    status.millis = millis();

    bool shouldWe = false;
    if (status.elapsed > FK_SETTINGS_TRANSMISSION_INTERVAL) {
        status.elapsed = 0;
        shouldWe = true;
    }

    write(&status);

    DEBUG_PRINT("TS: ");
    DEBUG_PRINT(status.time);
    DEBUG_PRINT(" ");
    DEBUG_PRINT(status.millis);
    DEBUG_PRINT(" ");
    DEBUG_PRINT(status.elapsed);
    DEBUG_PRINT(" ");
    DEBUG_PRINT(shouldWe);
    DEBUG_PRINTLN();

    return shouldWe;
}
