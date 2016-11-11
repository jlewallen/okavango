#include <SD.h>
#include "Platforms.h"
#include "core.h"
#include "UptimeTracker.h"

#define FK_SETTINGS_UPTIME_STATUS_FILENAME    "UPTIME.INF"
#define NUMBER_OF_UPTIMES                     10

typedef struct fk_uptime_slot_t {
    uint32_t uptime;
    uint32_t time;
    bool valid;
} fk_uptime_slot_t;

typedef struct fk_uptime_status_t {
    fk_uptime_slot_t startups[NUMBER_OF_UPTIMES];
} fk_uptime_status_t;

bool read(fk_uptime_status_t *status) {
    if (SD.exists(FK_SETTINGS_UPTIME_STATUS_FILENAME)) {
        File file = SD.open(FK_SETTINGS_UPTIME_STATUS_FILENAME, FILE_READ);
        if (!file) {
            DEBUG_PRINTLN(F("UT: unavailable"));
            return false;
        }
        if (file.size() != sizeof(fk_uptime_status_t)) {
            file.close();
            DEBUG_PRINTLN(F("UT: bad size "));
            SD.remove(FK_SETTINGS_UPTIME_STATUS_FILENAME);
            return false;
        }
        if (file.read((uint8_t *)status, sizeof(fk_uptime_status_t)) != sizeof(fk_uptime_status_t)) {
            file.close();
            DEBUG_PRINTLN(F("UT: read error"));
            SD.remove(FK_SETTINGS_UPTIME_STATUS_FILENAME);
            return false;
        }

        file.close();

        return true;
    }

    return false;
}

bool write(fk_uptime_status_t *status) {
    File file = SD.open(FK_SETTINGS_UPTIME_STATUS_FILENAME, FILE_WRITE);
    if (!file) {
        DEBUG_PRINTLN(F("UT: unavailable"));
        return false;
    }

    file.seek(0);

    if (file.write((uint8_t *)status, sizeof(fk_uptime_status_t)) != sizeof(fk_uptime_status_t)) {
        file.close();
        DEBUG_PRINTLN(F("UT: write error"));
        SD.remove(FK_SETTINGS_UPTIME_STATUS_FILENAME);
        return false;
    }

    file.flush();
    file.close();

    return true;
}

bool UptimeTracker::shouldWeRelax() {
    bool hadTroubleStayingUp = true;

    fk_uptime_status_t status;
    if (read(&status)) {
        for (uint8_t i = 0; i < NUMBER_OF_UPTIMES; ++i) {
            DEBUG_PRINT("UPTIME: ");
            DEBUG_PRINT(status.startups[i].valid);
            DEBUG_PRINT(" ");
            DEBUG_PRINT(status.startups[i].uptime);
            DEBUG_PRINT(" ");
            DEBUG_PRINT(status.startups[i].time);
            DEBUG_PRINTLN("");
            logPrinter.flush();

            if (i < 3) {
                if (status.startups[i].valid) {
                    hadTroubleStayingUp = hadTroubleStayingUp && status.startups[i].uptime == 0;
                }
                else {
                    hadTroubleStayingUp = false;
                }
            }
        }
    }

    if (hadTroubleStayingUp) {
        status.startups[0].valid = false;
        write(&status);
        return true;
    }

    return false;
}

void UptimeTracker::started() {
    fk_uptime_status_t status;
    memzero((uint8_t *)&status, sizeof(fk_uptime_status_t));

    if (!read(&status)) {
        memzero((uint8_t *)&status, sizeof(fk_uptime_status_t));
    }
    else {
        for (uint8_t i = NUMBER_OF_UPTIMES - 1; i > 0; --i) {
            status.startups[i].uptime = status.startups[i - 1].uptime;
            status.startups[i].time = status.startups[i - 1].time;
            status.startups[i].valid = status.startups[i - 1].valid;
        }
    }

    status.startups[0].valid = true;
    status.startups[0].uptime = 0;
    status.startups[0].time = SystemClock->now();

    write(&status);
}

void UptimeTracker::remember() {
    fk_uptime_status_t status;

    if (read(&status)) {
        status.startups[0].uptime = millis();
        status.startups[0].time = SystemClock->now();
        status.startups[0].valid = true; // Just in case.

        DEBUG_PRINT("UT: ");
        DEBUG_PRINT(status.startups[0].uptime);
        DEBUG_PRINT(" ");
        DEBUG_PRINTLN(status.startups[0].time);

        write(&status);
    }
}
