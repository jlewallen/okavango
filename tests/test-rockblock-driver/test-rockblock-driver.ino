#include "RockBlock.h"
#include "config.h"

RockBlock rockBlock("1,334.0,123,344.0,3243.0,3243");

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
    while (!rockBlock.isDone() && !rockBlock.isFailed()) {
        rockBlock.tick();
        delay(10);
    }

    while (1) {
    }
}

// vim: set ft=cpp:
