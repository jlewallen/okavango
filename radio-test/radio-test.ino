#include "LoraRadio.h"

#define RFM95_CS  10
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

    if (millis() - lastSend > 2000) {
        char message[20] = "Hello World #      ";
        itoa(packetNumber++, message + 13, 10);
        message[19] = 0;

        Serial.print("Sending ");
        Serial.println(message);
        radio.send((uint8_t *)message, sizeof(message));

        // radio.printRegisters();
        lastSend = millis();
    }

    delay(10);
}

// vim: set ft=cpp:
