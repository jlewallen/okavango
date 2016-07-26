#define PIN 13

#include <SD.h>
#include <Arduino.h>
#include "AtlasScientific.h"

PhBoard phBoard(2, 3);

void setup() {
    delay(2000);

    Serial.begin(115200);      

    pinMode(PIN, OUTPUT);
    digitalWrite(PIN, LOW);

    phBoard.setup();

    Serial.println("\nSample...");

    phBoard.sample();

    phBoard.sleep();

    Serial.println("\nDone");
}

void loop() {
}
