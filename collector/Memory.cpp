#include "Memory.h"
#include "Diagnostics.h"

#include <system.h>
#include <nvm.h>
#include <sam.h>
#include <wdt.h>
#include <eeprom.h>

static bool eeprom_configure_fuses() {
    nvm_fusebits fb;

    if (nvm_get_fuses(&fb) == STATUS_OK) {
        if (fb.eeprom_size != NVM_EEPROM_EMULATOR_SIZE_1024) {
            fb.eeprom_size = NVM_EEPROM_EMULATOR_SIZE_1024;
            if (nvm_set_fuses(&fb) == STATUS_OK) {
                Serial.println("Changed EEPROM Size:");
                Serial.println(fb.eeprom_size);
                return true;
            }
            else {
                Serial.println("Unable to set fuses.");
            }

        }
        else {
            Serial.println("EEPROM Size:");
            Serial.println(fb.eeprom_size);
            return true;
        }
    }
    else {
        Serial.println("Unable to get fuses.");
    }

    return false;
}

static status_code eeprom_emulation_configure(void) {
    enum status_code error_code = eeprom_emulator_init();
    if (error_code == STATUS_ERR_NO_MEMORY) {
        if (eeprom_configure_fuses()) {
            NVIC_SystemReset();
        }

        return STATUS_ERR_NO_MEMORY;
    }
    else if (error_code != STATUS_OK) {
        /* Erase the emulated EEPROM memory (assume it is unformatted or
         * irrecoverably corrupt) */
        Serial.println("memory: Erasing, remember programming erases all FLASH memory.");
        eeprom_emulator_erase_memory();
        eeprom_emulator_init();
    }

    return error_code;
}

uint32_t msToInterval(uint32_t ms) {
    return ms;
}

uint32_t intervalToMs(uint32_t interval) {
    return interval;
}

void Memory::setup() {
    Serial.print("Memory size: ");
    Serial.print(sizeof(fk_memory_state_t));
    Serial.print(" / ");
    Serial.println(EEPROM_PAGE_SIZE);

    switch (eeprom_emulation_configure()) {
    case STATUS_ERR_NO_MEMORY:
        // This is very bad.
        break;
    case STATUS_OK:
        uint8_t page[EEPROM_PAGE_SIZE];
        eeprom_emulator_read_page(0, page);
        memcpy((uint8_t *)&state, page, sizeof(fk_memory_state_t));
        initialized = true;
        break;
    case STATUS_ERR_IO:
    case STATUS_ERR_BAD_FORMAT:
    default:
        memset((uint8_t *)&state, 0, sizeof(fk_memory_state_t));

        #define IDLE_PERIOD                            (1000 * 60 * 10)
        #define AIRWAVES_CHECK_TIME                    (1000 * 60 * 10)
        #define WEATHER_STATION_CHECK_TIME             (1000 * 10)
        #define MANDATORY_RESTART_INTERVAL             (1000 * 60 * 60 * 6)

        state.intervals.idle = msToInterval(IDLE_PERIOD);
        state.intervals.airwaves = msToInterval(AIRWAVES_CHECK_TIME);
        state.intervals.weather = msToInterval(WEATHER_STATION_CHECK_TIME);
        state.intervals.restart = msToInterval(MANDATORY_RESTART_INTERVAL);

        #define WEATHER_STATION_INTERVAL_START         (1000 * 60)
        #define WEATHER_STATION_INTERVAL_IGNORE        (1000 * 60 * 30)
        #define WEATHER_STATION_INTERVAL_OFF           (1000 * 60 * 30)
        #define WEATHER_STATION_INTERVAL_READING       (1000 * 1000 * 60 * 2)

        state.intervals.weatherStation.start = msToInterval(WEATHER_STATION_INTERVAL_START);
        state.intervals.weatherStation.ignore = msToInterval(WEATHER_STATION_INTERVAL_IGNORE);
        state.intervals.weatherStation.off = msToInterval(WEATHER_STATION_INTERVAL_OFF);
        state.intervals.weatherStation.reading = msToInterval(WEATHER_STATION_INTERVAL_READING);

        fk_transmission_schedule_t default_schedules[TRANSMISSION_KIND_KINDS] = {
            { 24, 24 }, // Location
            { 0,  12 }, // Sensors
            { 2,  12 }  // Weather
        };

        memcpy(&state.schedules, &default_schedules, sizeof(default_schedules));

        break;
    }
}

void Memory::save() {
    uint8_t page[EEPROM_PAGE_SIZE];
    memcpy((uint8_t *)page, (uint8_t *)&state, sizeof(fk_memory_state_t));
    eeprom_emulator_write_page(0, page);
    eeprom_emulator_commit_page_buffer();
}

void Memory::update(String name) {
    size_t len = min(name.length(), 2);
    if (strncmp(state.name, name.c_str(), len) != 0) {
        memcpy(state.name, name.c_str(), len);
        state.name[2] = 0;
        save();
        Serial.println("memory: Name updated");
    }
}

void Memory::restarting() {
    state.restarts++;
    state.restartAt = SystemClock->now();
    save();
}

void Memory::markAlive(uint32_t time) {
    if (state.dyingAt > 0) {
        diagnostics.recordDeadFor(time - state.dyingAt);
        state.dyingAt = 0;
        state.aliveAt = time;
        save();
    }
}

void Memory::markDying(uint32_t time) {
    state.dyingAt = time;
    save();
}
