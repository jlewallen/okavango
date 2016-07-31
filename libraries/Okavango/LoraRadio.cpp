#include "LoraRadio.h"

#define RF95_FREQ 915.0
#define HALT(error) Serial.println(error); while (true) { }

LoraRadio::LoraRadio(uint8_t pinCs, uint8_t pinG0, uint8_t pinEnable)
    : rf95(pinCs, pinG0), pinEnable(pinEnable) {
}

void LoraRadio::setup() {
    pinMode(pinEnable, OUTPUT);

    powerOn();
    delay(10);
    reset();

    while (!rf95.init()) {
        HALT("LoraRadio: Initialize failed!");
    }

    if (!rf95.setFrequency(RF95_FREQ)) {
        HALT("LoraRadio: setFrequency failed!");
    }

    rf95.setTxPower(23, false);
}

void LoraRadio::send(uint8_t *packet, uint8_t size) {
    rf95.send(packet, size);
}

void LoraRadio::tick() {
    if (rf95.available()) {
        uint8_t length = sizeof(buffer);
        rf95.recv(buffer, &length);
    }
}

