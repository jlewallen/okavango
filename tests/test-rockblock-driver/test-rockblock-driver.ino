#include "RockBlock.h"
#include "config.h"

RockBlock rockBlock("");

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

    Serial1.begin(19200);

    Serial.println("Starting...");

    SerialType &rockBlockSerial = Serial1;
    rockBlock.setSerial(&rockBlockSerial);
}

void loop() {
    rockBlock.tick();
}

// vim: set ft=cpp:
