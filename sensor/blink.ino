#include "other.h"

#define PIN 13

Something something();

void setup() {
    pinMode(PIN, OUTPUT);
}

void loop() {
    digitalWrite(PIN, HIGH);
    delay(100);
    digitalWrite(PIN, LOW);
    delay(100);
}
