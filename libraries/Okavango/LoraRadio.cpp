#include "LoraRadio.h"

#define RF95_FREQ 915.0

LoraRadio::LoraRadio(uint8_t pinCs, uint8_t pinG0, uint8_t pinEnable)
    : rf95(pinCs, pinG0), pinEnable(pinEnable), available(false) {
}

bool LoraRadio::setup() {
    pinMode(pinEnable, OUTPUT);

    powerOn();
    delay(10);
    reset();

    if (!rf95.init()) {
        Serial.println("LoraRadio: Initialize failed!");
        return false;
    }

    if (!rf95.setFrequency(RF95_FREQ)) {
        Serial.println("LoraRadio: setFrequency failed!");
        return false;
    }

    rf95.setTxPower(23, false);

    available = true;
    return true;
}

bool LoraRadio::send(uint8_t *packet, uint8_t size) {
    return rf95.send(packet, size);
}

void LoraRadio::tick() {
    if (rf95.available()) {
        length = sizeof(buffer);
        rf95.recv(buffer, &length);
    }
}

