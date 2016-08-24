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
    DEBUG_PRINT("TS: ");
    DEBUG_PRINT(status->time);
    DEBUG_PRINTLN("");

    for (int8_t i = 0; i < TRANSMISSION_KIND_KINDS; ++i) {
        DEBUG_PRINT(i);
        DEBUG_PRINT(": acc=");
        DEBUG_PRINT(status->kinds[i].elapsed);
        DEBUG_PRINT(", prv=");
        DEBUG_PRINT(status->kinds[i].millis);
        DEBUG_PRINT(", dT=");
        DEBUG_PRINT(millis() - status->kinds[i].millis);
        DEBUG_PRINT(", rem=");
        DEBUG_PRINT(TransmissionIntervals[i] - status->kinds[i].elapsed);
        DEBUG_PRINTLN();
    }
}

void TransmissionStatus::dump() {
    fk_transmission_status_t status;

    if (!read(&status)) {
        DEBUG_PRINTLN("TS: Nothing to dump.");
        return;
    }

    log(&status);
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

    DEBUG_PRINTLN("");

    int8_t which = -1;
    for (int8_t i = 0; i < TRANSMISSION_KIND_KINDS; ++i) {
        int32_t change = now > status.kinds[i].millis ? now - status.kinds[i].millis : 0;

        DEBUG_PRINT("TS #");
        DEBUG_PRINT(i);
        DEBUG_PRINT(": acc=");
        DEBUG_PRINT(status.kinds[i].elapsed);
        DEBUG_PRINT(", prv=");
        DEBUG_PRINT(status.kinds[i].millis);
        DEBUG_PRINT(", dT=");
        DEBUG_PRINT(change);
        DEBUG_PRINT(", rem=");
        DEBUG_PRINT(TransmissionIntervals[i] - status.kinds[i].elapsed);
        DEBUG_PRINTLN();

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
