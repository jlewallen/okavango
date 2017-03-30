#include <SD.h>
#include "Platforms.h"
#include "Config.h"

bool Config::read() {
    bool success = true;

    File urlFile = SD.open("URL.CFG");
    if (urlFile) {
        url = urlFile.readString();
        url.trim();
        urlFile.close();

        DEBUG_PRINT("URL: ");
        DEBUG_PRINTLN(url);
    }
    else {
        DEBUG_PRINTLN("No url.cfg");
        success = false;
    }

    File wifiFile = SD.open("WIFI.CFG");
    if (wifiFile) {
        String data = wifiFile.readString();
        int32_t i = data.indexOf(' ');
        if (i > 0) {
            ssid = data.substring(0, i);
            password = data.substring(i);

            ssid.trim();
            password.trim();

            DEBUG_PRINT("Wifi: ");
            DEBUG_PRINT(ssid);
            DEBUG_PRINT(" ");
            DEBUG_PRINTLN(password);
        }
        else {
            success = false;
        }

        wifiFile.close();
    }
    else {
        DEBUG_PRINTLN("No wifi.cfg");
        success = false;
    }

    return success;
}
