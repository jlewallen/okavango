#include <SD.h>

#include "Platforms.h"
#include "TransmissionStatus.h"
#include "core.h"

typedef struct fk_transmission_kind_status_t {
    uint32_t millis;
    uint32_t elapsed;
    uint32_t time;
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
        DEBUG_PRINT(", time=");
        DEBUG_PRINT(status->kinds[i].time);
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

    // We use 0 here until we're initialized so that we can be sure to assign
    // a propert time. Then we set 0's to a good time. This will cause problems
    // if we go long periods of time w/o getting a GPS fix at startup.
    uint32_t rtcNow = SystemClock.initialized() ? SystemClock.now() : 0;
    uint32_t millisNow = millis();

    if (!read(&status)) {
        memzero((uint8_t *)&status, sizeof(fk_transmission_status_t));
        for (int8_t i = 0; i < TRANSMISSION_KIND_KINDS; ++i) {
            status.kinds[i].millis = millisNow;
            status.kinds[i].elapsed = 0;
            status.kinds[i].time = rtcNow;
        }
    }

    status.time = rtcNow;

    // Assign a proper now to those that are uninitialized.
    for (int8_t i = 0; i < TRANSMISSION_KIND_KINDS; ++i) {
        if (status.kinds[i].time == 0) {
            DEBUG_PRINT("TS #");
            DEBUG_PRINT(i);
            DEBUG_PRINTLN(" fixing 0 value uninitialized time.");
            status.kinds[i].time = rtcNow;
        }
    }

    DEBUG_PRINTLN("");

    int8_t which = -1;
    for (int8_t i = 0; i < TRANSMISSION_KIND_KINDS; ++i) {
        // I was worried about some kind of weird situation where we were always
        // 0 here. Or even usually 0 here. If millis() is less than the old
        // millis we know that millis() has passed since, so that works.
        uint32_t change = millisNow > status.kinds[i].millis ? (millisNow - status.kinds[i].millis) : millis();

        DEBUG_PRINT("TS #");
        DEBUG_PRINT(i);
        DEBUG_PRINT(": acc=");
        DEBUG_PRINT(status.kinds[i].elapsed);
        DEBUG_PRINT(", prv=");
        DEBUG_PRINT(status.kinds[i].millis);
        DEBUG_PRINT(", time=");
        DEBUG_PRINT(status.kinds[i].time);
        DEBUG_PRINT(", dT=");
        DEBUG_PRINT(change);
        DEBUG_PRINT(", rem=");
        DEBUG_PRINT(TransmissionIntervals[i] - status.kinds[i].elapsed);
        DEBUG_PRINTLN();

        if (change > 0) { // Should never be < 0, but meh.
            status.kinds[i].elapsed += change;
        }
        status.kinds[i].millis = millisNow;

        uint32_t intervalMs = TransmissionIntervals[i];

        // Don't bother if this time is 0, and therefore uninitialized.
        int32_t rtcElapsed = status.kinds[i].time > 0 ? rtcNow - status.kinds[i].time : 0;

        if (status.kinds[i].elapsed > intervalMs || rtcElapsed > (intervalMs / 1000)) {
            // If we don't 0 we'll get done next time.
            if (which < 0) {
                DEBUG_PRINT("Trigger #");
                DEBUG_PRINT(i);
                DEBUG_PRINT(": ");
                DEBUG_PRINT(rtcNow - status.kinds[i].time);
                DEBUG_PRINT(" ");
                DEBUG_PRINT(status.kinds[i].elapsed);
                DEBUG_PRINTLN();
                status.kinds[i].elapsed = 0;
                status.kinds[i].time = rtcNow;
                which = i;
            }
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

// vim: set ft=cpp:
