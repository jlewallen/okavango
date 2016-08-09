#include "Platforms.h"
#include "LoraRadio.h"

LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST);

void setup() {
    Serial.begin(115200);

    // Unnecessary for LoRa, though the SD seems to need this in some situations.
    // pinMode(RFM95_CS, OUTPUT);
    // digitalWrite(RFM95_CS, HIGH);

    delay(5000);

    Serial1.begin(9600);

    while (!Serial) {
        delay(100);
        if (millis() > 30 * 1000) {
            break;
        }
    }

    if (!radio.setup()) {
        Serial.println(F("Radio missing."));
    }
    Serial.println(F("Ready."));
}

int32_t freeRam() {
    extern int __heap_start, *__brkval; 
    int v; 
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void loop() {
    if (Serial1.available()) {
        bool reading = true;
        delay(100);
        while (reading) {
            int32_t c = Serial1.read();
            if (c > 0) {
                if (c == '\n') {
                    Serial.println();
                    Serial.println(freeRam());
                    break;
                }
                else {
                    Serial.print((char)c);
                }
            }
        }
    }

    radio.tick();

    if (radio.hasPacket()) {
        Serial.println("Has packet ***********************");
        radio.clear();
    }

    delay(10);
}

// vim: set ft=cpp:
