#include <SD.h>

#include "Configuration.h"
#include "Platforms.h"

Configuration::Configuration(const char *filename) : filename(filename) {
}

bool Configuration::read() {
    File file = SD.open(filename);
    if (!file) {
        DEBUG_PRINTLN("No configuration");
        return false;
    }

    String data = file.readString();
    int32_t i = data.indexOf(' ');
    if (i <= 0) {
        DEBUG_PRINTLN("Malformed configuration");
        return false;
    }

    String fonaOrRb = data.substring(i + 1, data.length());
    fonaOrRb.trim();

    name = data.substring(0, i);
    hasFona = fonaOrRb == "FONA";
    hasRockBlock = fonaOrRb == "ROCKBLOCK";

    DEBUG_PRINT("Config: ");
    DEBUG_PRINT(name);
    DEBUG_PRINT(" ");
    DEBUG_PRINT(hasFona);
    DEBUG_PRINT(" ");
    DEBUG_PRINT(hasRockBlock);
    DEBUG_PRINTLN("");

    file.close();
    
    return true;
}
