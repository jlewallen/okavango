#include <Arduino.h>
#include "AtlasScientific.h"
#include "SerialPortExpander.h"

#include <SPI.h>
#include <RH_RF95.h>

SerialPortExpander portExpander(6, 7, 8);
SoftwareSerial portExpanderSerial(2, 3);
SoftwareSerial conductivitySerial(4, 5);
AtlasScientificBoard board;
uint8_t selectedPort = 0;

void finish() {
    portExpander.select(3);
    while (1) {
        delay(1000);
    }
}

void setup() {
    delay(3000);

    Serial.begin(115200);      
    Serial.println("Begin");

    portExpander.setup();
    portExpander.select(selectedPort);
    board.setSerial(&portExpanderSerial);
    board.start();
}

void loop() {
    board.tick();
    delay(50);

    if (board.isDone()) {
        selectedPort++;
        if (selectedPort < 3) {
            Serial.println("Next sensor");
            portExpander.select(selectedPort);
            board.start();
        }
        else if (selectedPort == 3) {
            Serial.println("Conductivity");
            board.setSerial(&conductivitySerial);
            board.start();
        }
        else {
            Serial.println("Done");
            finish();
        }
    }
}
