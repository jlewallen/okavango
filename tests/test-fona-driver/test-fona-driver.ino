#include "fona.h"
#include "config.h"

FonaChild fona(NUMBER_TO_SMS);

void setup() {
    Serial.begin(115200);

    #ifdef WAIT_FOR_SERIAL
    while (!Serial) {
        delay(100);
        if (millis() > WAIT_FOR_SERIAL) {
            break;
        }
    }
    #endif

    Serial1.begin(4800);

    SerialType &fonaSerial = Serial1;
    fona.setSerial(&fonaSerial);
}

void loop() {
    fona.tick();
}

// vim: set ft=cpp:
