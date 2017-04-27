#include <Arduino.h>
#include <SerialPortExpander.h>

const uint8_t PIN_SPE_ISO_SEL0 = 14;
const uint8_t PIN_SPE_ISO_SEL1 = 15;
const uint8_t PIN_SPE_SEL0 = 16;
const uint8_t PIN_SPE_SEL1 = 17;

SerialPortExpander speIsolated(PIN_SPE_ISO_SEL0, PIN_SPE_ISO_SEL1, ConductivityConfig::None, &Serial2);
SerialPortExpander speNormal(PIN_SPE_SEL0, PIN_SPE_SEL1, ConductivityConfig::None, &Serial1);

void setup() {
    Serial.begin(115200);

    while (!Serial && millis() < 5000) {
        delay(100);
    }

    Serial.println("Ready..");

    speIsolated.setup();
    speNormal.setup();
}

void loop() {
    uint32_t lastChange = millis();
    uint8_t position = 0;
    uint8_t passes = 0;

    SerialType *serial = nullptr;

    while (true) {
        if (serial == nullptr || millis() - lastChange > 10 * 1000) {
            lastChange = millis();

            switch (position) {
            case 0:
            case 1:
            case 2:
            case 3:
                Serial.print("############## On SpeIsolated ");
                Serial.print(passes);
                Serial.print(" ");
                Serial.println(position);
                speIsolated.select(position);
                serial = speIsolated.getSerial();
                break;
            case 4:
            case 5:
                Serial.print("############## On SpeNormal ");
                Serial.print(passes);
                Serial.print(" ");
                Serial.println(position - 4);
                speNormal.select(position - 4);
                serial = speNormal.getSerial();
                break;
            }

            while (serial->available()) {
                serial->read();
            }

            if (position < 5) {
                Serial.println("Configure");
                // serial->print("FACTORY\r");
                serial->print("C,1\r");
                if ((passes % 2) == 0) {
                    // serial->print("L,1\r");
                }
                else {
                    serial->print("L,0\r");
                }
            }

            position = (position + 1) % 6;

            if (position == 0) {
                passes++;
            }
        }

        while (serial->available()) {
            char c = (char)serial->read();
            Serial.print(c);
            if (c == '\r') {
                Serial.print("\n");
            }
        }

        delay(10);
    }
}
