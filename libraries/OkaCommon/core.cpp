#include "core.h"
#include "protocol.h" 

CorePlatform::CorePlatform()
    : sdLogger(PIN_SD_CS), loraRadio(RFM95_CS, RFM95_INT, RFM95_RST), queue(PIN_SD_CS, FK_QUEUE_ENTRY_SIZE, FK_SETTINGS_QUEUE_FILENAME) {
}

void CorePlatform::setup() {
    pinMode(PIN_RED_LED, OUTPUT);
    digitalWrite(PIN_RED_LED, HIGH);
    pinMode(PIN_GREEN_LED, OUTPUT);
    digitalWrite(PIN_GREEN_LED, LOW);

    pinMode(RFM95_CS, OUTPUT);
    pinMode(RFM95_RST, OUTPUT);
    digitalWrite(RFM95_CS, HIGH);
    digitalWrite(RFM95_RST, HIGH);

    pinMode(PIN_SD_CS, OUTPUT);
    digitalWrite(PIN_SD_CS, HIGH);

    if (!rtc.begin()) {
        Serial.println("RTC Missing");
    }
    else {
        if (!rtc.initialized()) {
            Serial.println("RTC uninitialized");
            rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }
    }

    if (!sdLogger.setup()) {
        platformCatastrophe(PIN_RED_LED);
    }

    if (loraRadio.setup()) {
        loraRadio.sleep();
    }
}

bool CorePlatform::tick() {
    return false;
}

void CorePlatform::enqueue(uint8_t *data) {
    queue.enqueue(data);
}

uint8_t *CorePlatform::dequeue() {
    return queue.dequeue();
}

