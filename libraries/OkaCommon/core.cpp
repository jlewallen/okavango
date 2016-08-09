#include "core.h"
#include "protocol.h" 

CorePlatform::CorePlatform(fk_board_t *board) :
    sdLogger(),
    loraRadio(board->pin_rfm95_cs, board->pin_rfm95_g0, board->pin_rfm95_power),
    localQueue(FK_QUEUE_ENTRY_SIZE, FK_SETTINGS_QUEUE_FILENAME) {
}

void CorePlatform::setup() {
    pinMode(PIN_RED_LED, OUTPUT);
    digitalWrite(PIN_RED_LED, LOW);
    pinMode(PIN_GREEN_LED, OUTPUT);
    digitalWrite(PIN_GREEN_LED, LOW);

    pinMode(RFM95_CS, OUTPUT);
    pinMode(RFM95_RST, OUTPUT);
    digitalWrite(RFM95_CS, HIGH);
    digitalWrite(RFM95_RST, HIGH);

    pinMode(PIN_SD_CS, OUTPUT);
    digitalWrite(PIN_SD_CS, HIGH);

    #ifdef FEATHER_WING_ADALOGGER
    if (!rtc.begin()) {
        Serial.println("RTC Missing");
    }
    else {
        if (!rtc.initialized()) {
            Serial.println("RTC uninitialized");
            rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }
    }
    #endif

    pinMode(PIN_SD_CS, OUTPUT);

    if (SD.begin(PIN_SD_CS)) {
        sdLogger.setup();
        localQueue.setup();
    }
    else {
        Serial.println("SD Missing");
    }

    if (!loraRadio.setup()) {
        Serial.println("Radio Missing");
    }
    else {
        loraRadio.sleep();
    }
}

bool CorePlatform::tick() {
    return false;
}

void CorePlatform::enqueue(uint8_t *data) {
    localQueue.enqueue(data);
}

uint8_t *CorePlatform::dequeue() {
    return localQueue.dequeue();
}

