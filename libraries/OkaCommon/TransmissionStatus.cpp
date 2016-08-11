#include <SD.h>

#include "Platforms.h"
#include "TransmissionStatus.h"
#include "core.h"

typedef struct fk_transmission_kind_status_t {
    uint32_t millis;
    uint32_t elapsed;
} fk_transmission_kind_status_t;

typedef struct fk_transmission_status_t {
    fk_transmission_kind_status_t kinds[TRANSMISSION_KIND_KINDS];
    uint32_t time;
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

int8_t TransmissionStatus::shouldWe() {
    fk_transmission_status_t status;

    if (!read(&status)) {
        memzero((uint8_t *)&status, sizeof(fk_transmission_status_t));
        status.time = SystemClock.now();
        for (int8_t i = 0; i < TRANSMISSION_KIND_KINDS; ++i) {
            status.kinds[i].millis = millis();
            status.kinds[i].elapsed = 0;
        }
    }

    int8_t which = -1;
    for (int8_t i = 0; i < TRANSMISSION_KIND_KINDS; ++i) {
        status.kinds[i].elapsed += millis() - status.kinds[i].millis;
        status.kinds[i].millis = millis();

        if (status.kinds[i].elapsed > TransmissionIntervals[i]) {
            status.kinds[i].elapsed = 0;
            which = i;
            break;
        }
    }

    write(&status);

    DEBUG_PRINT("TS: ");
    DEBUG_PRINT(status.time);
    DEBUG_PRINT(" ");
    DEBUG_PRINT(which);
    DEBUG_PRINTLN();

    return which;
}
