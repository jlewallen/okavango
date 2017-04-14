#include <SD.h>

#include "Configuration.h"
#include "Platforms.h"

Configuration::Configuration(Memory *memory, const char *filename) :
    memory(memory), filename(filename) {
}

bool Configuration::read(bool sdAvailable) {
    if (sdAvailable) {
        File file = SD.open(filename);
        if (!file) {
            DEBUG_PRINTLN("No configuration");
            return false;
        }

        String data = file.readString();
        int32_t i = data.indexOf(' ');
        // New configuration can just be a name.
        if (i <= 0) {
            name = data;
        }
        else {
            // Support old configuration.
            name = data.substring(0, i);
        }

        memory->update(name);

        file.close();
    }
    else {
        if (!memory->isInitialized()) {
            return false;
        }

        name = memory->getName();
    }

    DEBUG_PRINT("Config: ");
    DEBUG_PRINT(name);
    DEBUG_PRINTLN("");

    return name.length() == 2;
}
