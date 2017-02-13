#include "Memory.h"

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

void Memory::setup() {
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
        break;
    }
}

void Memory::update(String name) {
    size_t len = min(name.length(), 2);
    if (strncmp(state.name, name.c_str(), len) != 0) {
        uint8_t page[EEPROM_PAGE_SIZE];
        memcpy(state.name, name.c_str(), len);
        state.name[2] = 0;
        memcpy((uint8_t *)page, (uint8_t *)&state, sizeof(fk_memory_state_t));
        eeprom_emulator_write_page(0, page);
        eeprom_emulator_commit_page_buffer();
        Serial.println("memory: Name updated");
    }
}
