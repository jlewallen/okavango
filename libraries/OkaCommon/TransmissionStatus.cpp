#include <SD.h>

#include "Platforms.h"
#include "TransmissionStatus.h"
#include "core.h"

typedef struct fk_transmission_kind_status_t {
    uint32_t millis;
    uint32_t time;
    uint32_t previousHour;
} fk_transmission_kind_status_t;

typedef struct fk_transmission_status_t {
    fk_transmission_kind_status_t kinds[TRANSMISSION_KIND_KINDS];
} fk_transmission_status_t;

fk_transmission_status_t status = { 0 };

bool TransmissionStatus::anyTransmissionsThisHour() {
    DateTime dt(SystemClock->now());
    for (int8_t i = 0; i < TRANSMISSION_KIND_KINDS; ++i) {
        if (status.kinds[i].previousHour == dt.hour() + 1) {
            return true;
        }
    }
    return false;
}

int8_t TransmissionStatus::shouldWe(fk_transmission_schedule_t *schedules) {
    uint32_t rtcNow = SystemClock->now();
    DateTime dt(rtcNow);

    int8_t which = -1;
    for (int8_t i = 0; i < TRANSMISSION_KIND_KINDS; ++i) {
        int32_t hour = (dt.hour() + schedules[i].offset) % schedules[i].interval;
        int32_t triggered = hour == 0 && status.kinds[i].previousHour != (dt.hour() + 1);

        DEBUG_PRINT("TS: #");
        DEBUG_PRINT(i);
        DEBUG_PRINT(": +");
        DEBUG_PRINT(schedules[i].offset);
        DEBUG_PRINT(" % ");
        DEBUG_PRINT(schedules[i].interval);
        DEBUG_PRINT(" == ");
        DEBUG_PRINT(hour);
        DEBUG_PRINT(" previousHour=");
        DEBUG_PRINT(status.kinds[i].previousHour);
        DEBUG_PRINT(" hour=");
        DEBUG_PRINT(dt.hour());
        DEBUG_PRINT(" ");
        DEBUG_PRINT(triggered);
        DEBUG_PRINTLN();

        if (which == -1 && triggered) {
            status.kinds[i].previousHour = dt.hour() + 1;
            which = i;
        }
        // Clear the previousHour if we're in an hour that won't trigger. This
        // way if it's the same hour of the day everytime we'll still trigger.
        else if (hour != 0) {
            status.kinds[i].previousHour = 0;
        }
    }

    DEBUG_PRINT("TS: ");
    DEBUG_PRINT(which);
    DEBUG_PRINTLN();

    return which;
}

// vim: set ft=cpp:
