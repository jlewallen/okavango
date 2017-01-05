#include <SPI.h>
#include <SD.h>

#include "Platforms.h"
#include "NgDemo.h"

CorePlatform corePlatform;
Pcf8523SystemClock Clock;
NgDemo ngd;

void setup() {
    Watchdog.enable();

    Serial.begin(115200);

    #ifdef WAIT_FOR_SERIAL
    while (!Serial) {
        delay(100);
        if (millis() > WAIT_FOR_SERIAL) {
            break;
        }
    }
    #endif

    Serial.println("Begin");

    corePlatform.setup(PIN_SD_CS, PIN_RFM95_CS, PIN_RFM95_RST);

    Serial.println("Setting up...");

    SystemClock->setup();

    Serial.println("Clock ready...");

    if (!ngd.setup()) {
    }

    Serial.println("Preflight...");

    if (ngd.preflight()) {
    }

    Serial.println("Ready");
}

void loop() {
    delay(50);

    ngd.tick();
}

// vim: set ft=cpp:
