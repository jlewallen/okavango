#include "Platforms.h"
#include "Queue.h"

#define SENSORS_PACKET_NUMBER_VALUES 11

typedef struct sensors_packet_t {
    uint8_t kind;
    uint32_t time;
    float battery;
    float values[SENSORS_PACKET_NUMBER_VALUES];
} sensors_packet_t;

uint8_t valueIndex = 0;
sensors_packet_t packet;

void setup() {
    pinMode(PIN_RED_LED, OUTPUT);
    digitalWrite(PIN_RED_LED, HIGH);

    pinMode(PIN_GREEN_LED, OUTPUT);
    digitalWrite(PIN_GREEN_LED, LOW);

    Serial.begin(115200);

    #ifdef WAIT_FOR_SERIAL
    while (!Serial) {
        delay(100);
        if (millis() > WAIT_FOR_SERIAL) {
            break;
        }
    }
    #endif

    Serial.println("Loop");
}

void loop() {
    Queue queue(PIN_SD_CS, sizeof(sensors_packet_t), "queue.bin");

    if (queue.setup()) {
        Serial.println(queue.size());

        if (queue.size() > 20) {
            sensors_packet_t *dequeued;
            while ((dequeued = (sensors_packet_t *)queue.dequeue()) != NULL) {
                Serial.print("Dequeued ");
                Serial.println(dequeued->kind);
                free(dequeued);
            }
        }
        else {
            Serial.println("Queueing..");
            for (uint8_t i = 0; i < 10; ++i) {
                packet.kind = i;
                queue.enqueue((uint8_t *)&packet);
            }

            Serial.println("Done");

            Serial.println(queue.size());
        }

        queue.close();
    }

    while (1) {
        platformBlink(PIN_GREEN_LED);
    }

    // Queue(uint8_t pinCs, size_t entrySize, const char *filename);
}

// vim: set ft=cpp:
