#include <SD.h>
#include "core.h"
#include "protocol.h"

RtcSystemClock SystemClock;

void CorePlatform::setup(uint8_t pinSdCs, uint8_t pinRfm95Cs, uint8_t pinRfm95Rst, bool haveClock) {
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
    else {
        Serial.println("SD okay.");
    }

    pinMode(pinRfm95Rst, OUTPUT);
    digitalWrite(pinRfm95Rst, HIGH);

    if (haveClock) {
        SystemClock.setup();
    }
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

    available = true;
    #endif
    #endif

    return true;
}

bool RtcSystemClock::initialized() {
    return available && rtc.initialized();
}

uint32_t RtcSystemClock::now() {
    if (available) {
        return rtc.now().unixtime();
    }
    return 0;
}

bool RtcSystemClock::set(uint32_t newTime) {
    if (!available) {
        return false;
    }
    uint32_t before = RtcSystemClock::now();
    bool uninitialized = !rtc.initialized();
    DEBUG_PRINT("Clock Adjusted before=");
    DEBUG_PRINTLN(before);
    rtc.adjust(newTime);
    adjusted = newTime;
    return uninitialized;
}
