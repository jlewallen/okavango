#include <SD.h>

#include "Platforms.h"
#include "TransmissionStatus.h"
#include "core.h"

// #define DEBUG_TS

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

        file.close();

        return true;
    }

    return false;
}

bool write(fk_transmission_status_t *status) {
    File file = SD.open(FK_SETTINGS_TRANSMISSION_STATUS_FILENAME, FILE_WRITE);
    if (!file) {
        DEBUG_PRINTLN(F("TS: unavailable"));
        return false;
    }

    file.seek(0);

    if (file.write((uint8_t *)status, sizeof(fk_transmission_status_t)) != sizeof(fk_transmission_status_t)) {
        file.close();
        DEBUG_PRINTLN(F("TS: write error"));
        SD.remove(FK_SETTINGS_TRANSMISSION_STATUS_FILENAME);
        return false;
    }

    file.flush();
    file.close();

    return true;
}

void log(fk_transmission_status_t *status) {
    for (int8_t i = 0; i < TRANSMISSION_KIND_KINDS; ++i) {
        DEBUG_PRINT(i);
        DEBUG_PRINT(": ");
        DEBUG_PRINT(status->kinds[i].elapsed);
        DEBUG_PRINT(", ");
        DEBUG_PRINT(status->kinds[i].millis);
        DEBUG_PRINT(", ");
        DEBUG_PRINT(millis() - status->kinds[i].millis);
        DEBUG_PRINTLN();
    }
}

void TransmissionStatus::remove() {
    SD.remove(FK_SETTINGS_TRANSMISSION_STATUS_FILENAME);
}

int8_t TransmissionStatus::shouldWe() {
    fk_transmission_status_t status;

    uint32_t now = millis();

    if (!read(&status)) {
        memzero((uint8_t *)&status, sizeof(fk_transmission_status_t));
        for (int8_t i = 0; i < TRANSMISSION_KIND_KINDS; ++i) {
            status.kinds[i].millis = now;
            status.kinds[i].elapsed = 0;
        }
    }

    #ifdef DEBUG_TS
    log(&status);
    #endif

    int8_t which = -1;
    for (int8_t i = 0; i < TRANSMISSION_KIND_KINDS; ++i) {
        int32_t change = now > status.kinds[i].millis ? now - status.kinds[i].millis : 0;

        if (change > 0) {
            status.kinds[i].elapsed += change;
        }
        status.kinds[i].millis = now;

        if (status.kinds[i].elapsed > TransmissionIntervals[i]) {
            status.kinds[i].elapsed = 0;
            if (which < 0) {
                which = i;
            }
        }
    }

    #ifdef DEBUG_TS
    log(&status);
    #endif

    status.time = SystemClock.now();

    write(&status);

    DEBUG_PRINT("TS: ");
    DEBUG_PRINT(status.time);
    DEBUG_PRINT(" ");
    DEBUG_PRINT(which);
    DEBUG_PRINTLN();

    return which;
}

// vim: set ft=cpp:
