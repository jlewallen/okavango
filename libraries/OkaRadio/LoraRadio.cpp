#include "LoraRadio.h"
#include "Platforms.h"

#define MAX_RETRIES 3

LoraRadio::LoraRadio(uint8_t pinCs, uint8_t pinG0, uint8_t pinEnable, uint8_t pinRst)
    : pinCs(pinCs), pinRst(pinRst), rf95(pinCs, pinG0), pinEnable(pinEnable), available(false) {
}

bool LoraRadio::setup() {
    if (available) {
        return true;
    }

    pinMode(pinCs, OUTPUT);
    digitalWrite(pinCs, HIGH);

    pinMode(pinRst, OUTPUT);
    digitalWrite(pinRst, HIGH);

    pinMode(pinEnable, OUTPUT);
    digitalWrite(pinEnable, HIGH);

    powerOn();
    delay(10);
    reset();

    if (!rf95.init()) {
        DEBUG_PRINTLN(F("Radio missing"));
        return false;
    }

    if (!rf95.setFrequency(RF95_FREQ)) {
        DEBUG_PRINTLN(F("Radio setup failed"));
        return false;
    }

    rf95.setTxPower(23, false);

    available = true;
    return true;
}

bool LoraRadio::send(uint8_t *packet, uint8_t size) {
    memcpy(sendBuffer, packet, size);
    sendLength = size;
    tries = 0;
    return resend();
}

bool LoraRadio::reply(uint8_t *packet, uint8_t size) {
    rf95.setHeaderTo(rf95.headerFrom());
    bool success = send(packet, size);
    rf95.setHeaderTo(RH_BROADCAST_ADDRESS);
    return success;
}

bool LoraRadio::resend() {
    if (tries == MAX_RETRIES) {
        tries = 0;
        return false;
    }
    tries++;
    return rf95.send(sendBuffer, sendLength);
}

void LoraRadio::tick() {
    if (rf95.available()) {
        recvLength = sizeof(recvBuffer);
        rf95.recv(recvBuffer, &recvLength);
    }
    else {
        recvLength = 0;
    }
}
