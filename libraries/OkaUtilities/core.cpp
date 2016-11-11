#include <SD.h>
#include "core.h"
#include "protocol.h"

RtcSystemClock SystemClock;

void CorePlatform::setup(uint8_t pinSdCs, uint8_t pinRfm95Cs, uint8_t pinRfm95Rst) {
    pinMode(PIN_RED_LED, OUTPUT);
    digitalWrite(PIN_RED_LED, LOW);

    pinMode(PIN_GREEN_LED, OUTPUT);
    digitalWrite(PIN_GREEN_LED, LOW);

    // Important, the SDI CS' should be ready to go before we start using any
    // of them.
    pinMode(pinSdCs, OUTPUT);
    digitalWrite(pinSdCs, HIGH);

    pinMode(pinRfm95Cs, OUTPUT);
    digitalWrite(pinRfm95Cs, HIGH);

    if (!SD.begin(pinSdCs)) {
        DEBUG_PRINTLN(F("SD Missing"));
        platformCatastrophe(PIN_RED_LED, PLATFORM_CATASTROPHE_FAST_BLINK);
    }

    pinMode(pinRfm95Rst, OUTPUT);
    digitalWrite(pinRfm95Rst, HIGH);

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

bool RtcSystemClock::initialized() {
    return rtc.initialized();
}

uint32_t RtcSystemClock::now() {
    return rtc.now().unixtime();
}

bool RtcSystemClock::set(uint32_t newTime) {
    uint32_t before = RtcSystemClock::now();
    bool uninitialized = !rtc.initialized();
    DEBUG_PRINT("Clock Adjusted before=");
    DEBUG_PRINTLN(before);
    rtc.adjust(newTime);
    adjusted = newTime;
    return uninitialized;
}
