#include "Platforms.h"
#include "LoraRadio.h"

LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST, PIN_RFM95_RST);

bool broadcasting = false;

void setup() {
    pinMode(6, INPUT);

    pinMode(13, OUTPUT);
    digitalWrite(13, LOW);

    while (millis() < 2000 && !Serial) {
        delay(100);
    }

    Serial.begin(115200);
    delay(100);

    if (!radio.setup()) {
        platformCatastrophe(PIN_RED_LED);
    }

    Serial.println("LoRa Radio Demo");

    broadcasting = digitalRead(6);
}

uint16_t packetNumber = 0;
uint32_t lastReceived = 0;
uint32_t lastSend = 0;

void loop() {
    radio.tick();

    if (broadcasting) {
        digitalWrite(13, HIGH);

        if (millis() - lastSend > 2000) {
            char message[20] = "Hello World #      ";
            itoa(packetNumber++, message + 13, 10);
            message[19] = 0;

            Serial.print("Sending ");
            Serial.println(message);
            radio.send((uint8_t *)message, sizeof(message));

            lastSend = millis();

            digitalWrite(13, LOW);
            delay(100);
            digitalWrite(13, HIGH);
        }

    }
    else {
        if (radio.hasPacket()) {
            Serial.print("Packet: ");
            Serial.println((char *)radio.getPacket());
            radio.clear();
            lastReceived = millis();

            digitalWrite(13, HIGH);
            delay(100);
            digitalWrite(13, LOW);
        }
    }

    delay(10);
}

// vim: set ft=cpp:
