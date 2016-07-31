#ifndef LORA_RADIO_H
#define LORA_RADIO_H

#include <SPI.h>
#include <RH_RF95.h>

class LoraRadio {
private:
    RH_RF95 rf95;
    const uint8_t pinEnable;
    uint8_t buffer[RH_RF95_MAX_MESSAGE_LEN];

public:
    LoraRadio(uint8_t pinCs, uint8_t pinG0, uint8_t pinEnable);
    void setup();
    void tick();
    void send(uint8_t *packet, uint8_t size);

    const uint8_t *getPacket() {
        return buffer;
    }

    bool hasPacket() {
        return buffer[0] != 0;
    }

    bool isIdle() {
        return rf95.mode() == RHGenericDriver::RHMode::RHModeIdle;
    }

    void clear() {
        buffer[0] = 0;
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
