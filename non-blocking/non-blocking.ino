#include <Arduino.h>
#include "AtlasScientific.h"

PhBoard board(2, 3);

void setup() {
    delay(2000);

    Serial.begin(115200);      
    Serial.println("Begin");

    board.setup();

    Serial.println("Entering loop.");
}

void loop() {
    board.tick();

    delay(50);
}
