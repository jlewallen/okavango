#include <SD.h>
#include "core.h"
#include "protocol.h" 

RtcSystemClock SystemClock;

void CorePlatform::setup() {
    pinMode(PIN_RED_LED, OUTPUT);
    digitalWrite(PIN_RED_LED, LOW);

    pinMode(PIN_GREEN_LED, OUTPUT);
    digitalWrite(PIN_GREEN_LED, LOW);
    
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

    SystemClock.setup();
}

bool RtcSystemClock::setup() {
    #ifdef FEATHER_WING_ADALOGGER
    #ifndef FEATHER_DISABLE_RTC
    if (!rtc.begin()) {
        DEBUG_PRINTLN(F("RTC Missing"));
        return false;
    }
    else {
        if (!rtc.initialized()) {
            DEBUG_PRINTLN(F("RTC uninitialized"));
            rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }
    }
    #endif
    #endif

    return true;
}

uint32_t RtcSystemClock::now() {
    return rtc.now().unixtime();
}

bool RtcSystemClock::set(uint32_t now) {
    if (adjusted == 0) {
        DEBUG_PRINTLN("SystemClock Adjusted");
        rtc.adjust(now);
        adjusted = now;
        return true;
    }
    return false;
}
