#include <SD.h>

#include "Platforms.h"
#include "TransmissionStatus.h"
#include "core.h"

typedef struct fk_transmission_status_t {
    uint32_t time;
    uint32_t millis;
    uint32_t elapsed;
} fk_transmission_status_t;


bool TransmissionStatus::shouldWe() {
    CorePlatform corePlatform;

    File file = SD.open(FK_SETTINGS_TRANSMISSION_STATUS_FILENAME, FILE_WRITE);
    if (!file) {
        DEBUG_PRINTLN(F("Status unavailable"));
        return false;
    }

    fk_transmission_status_t status;

    file.seek(0);

    if (file.size() > 0) {
        if (file.read(&status, sizeof(fk_transmission_status_t)) != sizeof(fk_transmission_status_t)) {
            file.close();
            DEBUG_PRINTLN("Read TS error");
            SD.remove(FK_SETTINGS_TRANSMISSION_STATUS_FILENAME);
            return false;
        }
    }
    else {
        DEBUG_PRINTLN("New TS");
        status.time = corePlatform.now();
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


    if (file.write((uint8_t *)&status, sizeof(fk_transmission_status_t)) != sizeof(fk_transmission_status_t)) {
        file.close();
        DEBUG_PRINTLN("Write TS error");
        SD.remove(FK_SETTINGS_TRANSMISSION_STATUS_FILENAME);
        return false;
    }

    file.close();

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
