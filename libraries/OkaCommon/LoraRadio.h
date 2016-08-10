#ifndef LORA_RADIO_H
#define LORA_RADIO_H

#include <SPI.h>
#include <RH_RF95.h>
#include "protocol.h"

class LoraRadio {
private:
    RH_RF95 rf95;
    bool available;
    const uint8_t pinEnable;
    uint8_t sendBuffer[FK_QUEUE_ENTRY_SIZE];
    uint8_t sendLength;
    uint8_t recvBuffer[FK_QUEUE_ENTRY_SIZE];
    uint8_t recvLength;
    uint8_t tries;

public:
    LoraRadio(uint8_t pinCs, uint8_t pinG0, uint8_t pinEnable);
    bool setup();
    void tick();
    bool send(uint8_t *packet, uint8_t size);
    bool resend();
    uint8_t numberOfTries() {
        return tries;
    }
    bool isAvailable() {
        return available;
    }

    const uint8_t *getPacket() {
        return recvBuffer;
    }

    bool hasPacket() {
        return recvLength > 0;
    }

    bool isIdle() {
        return rf95.mode() == RHGenericDriver::RHMode::RHModeIdle;
    }

    void clear() {
        recvBuffer[0] = 0;
        recvLength = 0;
    }

    void wake() {
    }

    void powerOn() {
        digitalWrite(pinEnable, HIGH);
    }

    void powerOff() {
        digitalWrite(pinEnable, LOW);
    }

    void reset() {
        powerOff();
        delay(10);
        powerOn();
        delay(10);
    }

    void waitPacketSent() {
        rf95.waitPacketSent();
    }

    void sleep() {
        rf95.sleep();
    }

    void printRegisters() {
        rf95.printRegisters();
    }

    RHGenericDriver::RHMode mode() {
        return rf95.mode();
    }

    const char *modeName() {
        switch (mode()) {
        case RHGenericDriver::RHMode::RHModeInitialising: return "Initialising";
        case RHGenericDriver::RHMode::RHModeSleep: return "Sleep";
        case RHGenericDriver::RHMode::RHModeIdle: return "Idle";
        case RHGenericDriver::RHMode::RHModeTx: return "Tx";
        case RHGenericDriver::RHMode::RHModeRx: return "Rx";
        }
        return "Unknown";
    }
};

#endif