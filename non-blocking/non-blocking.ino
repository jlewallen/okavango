#include <Arduino.h>
#include "AtlasScientific.h"
#include "SerialPortExpander.h"

#include <SPI.h>
#include <RH_RF95.h>

SerialPortExpander serialPortExpander(5, 6, 7);
AtlasScientificBoard board(2, 3);
uint8_t selectedPort = 0;

void finish() {
    while (1) { }
}

void setup() {
    delay(3000);

    Serial.begin(115200);      
    Serial.println("Begin");

    serialPortExpander.setup();
    serialPortExpander.select(selectedPort);
    board.start();
}

void loop() {
    board.tick();
    delay(50);

    if (board.isDone()) {
        selectedPort++;
        if (selectedPort < 3) {
            Serial.println("Next sensor");
            serialPortExpander.select(selectedPort);
            board.start();
        }
        else {
            Serial.println("Done");
            finish();
        }
    }
}
