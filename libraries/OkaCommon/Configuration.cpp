#include "Configuration.h"
#include <SD.h>

Configuration::Configuration(const char *filename) : filename(filename) {
}

bool Configuration::read() {
    File file = SD.open(filename);
    if (!file) {
        Serial.println("No configuration");
        return false;
    }

    String data = file.readString();
    int32_t i = data.indexOf(' ');
    if (i <= 0) {
        Serial.println("Malformed configuration");
        return false;
    }

    String fonaOrRb = data.substring(i + 1, data.length());
    fonaOrRb.trim();

    name = data.substring(0, i);
    hasFona = fonaOrRb == "FONA";
    hasRockBlock = fonaOrRb == "ROCKBLOCK";

    Serial.print("Config: ");
    Serial.print(name);
    Serial.print(" ");
    Serial.print(hasFona);
    Serial.print(" ");
    Serial.print(hasRockBlock);
    Serial.println();

    file.close();
    
    return true;
}
