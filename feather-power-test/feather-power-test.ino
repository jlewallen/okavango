#include "Platforms.h"

void setup() {
}

void loop() {
    pinMode(13, OUTPUT);

    for (uint8_t i = 0; i < 10; ++i) {
        digitalWrite(13, HIGH);
        delay(500);
        digitalWrite(13, LOW);
        delay(500);
    }

    for (uint8_t i = 0; i < 10; ++i) {
        delay(1000);
    }

    platformLowPowerSleep(10 * 1000);
}

// vim: set ft=cpp:
