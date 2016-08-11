#include "RockBlock.h"
#include "config.h"

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

}

void loop() {
    String message = "1470945736,0.00,1020.00,0.00,0.00,0.00,234.59";
    for (uint8_t i = 0; i < 30; ++i) {
        RockBlock rockBlock(message);
        SerialType &rockBlockSerial = Serial1;
        rockBlock.setSerial(&rockBlockSerial);
        while (!rockBlock.isDone() && !rockBlock.isFailed()) {
            rockBlock.tick();
            delay(10);
        }
        message += "0";
    }


    while (1) {
    }
}

// vim: set ft=cpp:
