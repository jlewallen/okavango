#include <SD.h>
#include <Arduino.h>
#include "AtlasScientific.h"

PhBoard phBoard(2, 3);

void setup() {
    delay(2000);

    Serial.begin(115200);      

    phBoard.setup();

    Serial.println("Sample...");

    phBoard.sample();
    phBoard.sleep();

    Serial.println("Done!");
}

void loop() {
}
