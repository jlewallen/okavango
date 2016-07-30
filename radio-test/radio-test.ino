#include <SPI.h>
#include <RH_RF95.h>

#define RF95_FREQ 915.0

#define HALT(error) Serial.println(error); while (true) { }

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

    void sleep() {
        rf95.sleep();
    }
};

LoraRadio::LoraRadio(uint8_t pinCs, uint8_t pinG0, uint8_t pinEnable)
    : rf95(pinCs, pinG0), pinEnable(pinEnable) {
}

void LoraRadio::setup() {
    pinMode(pinEnable, OUTPUT);
    digitalWrite(pinEnable, HIGH);

    delay(10);
    digitalWrite(pinEnable, LOW);
    delay(10);
    digitalWrite(pinEnable, HIGH);
    delay(10);

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
    if (rf95.available())
    {
        uint8_t length = sizeof(buffer);
        rf95.recv(buffer, &length);
    }
}

#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2

LoraRadio radio(RFM95_CS, RFM95_INT, RFM95_RST);

void setup() {
    while (!Serial) {
        delay(100);
    }

    Serial.begin(115200);
    delay(100);

    radio.setup();

    Serial.println("LoRa Radio Demo");
}

uint16_t packetNumber = 0;
uint32_t lastReceived = 0;
uint32_t lastSend = 0;

void loop() {
    radio.tick();

    if (radio.hasPacket()) {
        Serial.print("Packet: ");
        Serial.println((char *)radio.getPacket());
        radio.clear();
        lastReceived = millis();
    }

    if (millis() - lastReceived > 10000) {
        if (radio.isIdle()) {
            char message[20] = "Hello World #      ";
            itoa(packetNumber++, message + 13, 10);
            Serial.print("Sending ");
            message[19] = 0;
            radio.send((uint8_t *)message, sizeof(message));
            lastSend = millis();
        }
    }

    delay(10);
}

// vim: set ft=cpp:
