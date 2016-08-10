#include <SD.h>
#include "core.h"
#include "protocol.h" 

void CorePlatform::setup() {
    pinMode(PIN_RED_LED, OUTPUT);
    digitalWrite(PIN_RED_LED, LOW);

    pinMode(PIN_GREEN_LED, OUTPUT);
    digitalWrite(PIN_GREEN_LED, LOW);

    #ifdef FEATHER_WING_ADALOGGER
    #ifndef FEATHER_DISABLE_RTC
    if (!rtc.begin()) {
        DEBUG_PRINTLN(F("RTC Missing"));
    }
    else {
        if (!rtc.initialized()) {
            DEBUG_PRINTLN(F("RTC uninitialized"));
            rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }
    }
    #endif
    #endif
    
    pinMode(PIN_SD_CS, OUTPUT);
    digitalWrite(PIN_SD_CS, HIGH);

    pinMode(PIN_RFM95_CS, OUTPUT);
    digitalWrite(PIN_RFM95_CS, HIGH);

    if (!SD.begin(PIN_SD_CS)) {
        DEBUG_PRINTLN(F("SD Missing"));
        platformCatastrophe(PIN_RED_LED);
    }

    pinMode(PIN_RFM95_RST, OUTPUT);
    digitalWrite(PIN_RFM95_RST, HIGH);
}

uint32_t CorePlatform::now() {
    return rtc.now().unixtime();
}
