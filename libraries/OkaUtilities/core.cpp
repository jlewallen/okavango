#include <SD.h>
#include "core.h"
#include "protocol.h"

RtcAbstractSystemClock *SystemClock;

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
    else {
        Serial.println("SD okay.");
    }

    pinMode(pinRfm95Rst, OUTPUT);
    digitalWrite(pinRfm95Rst, HIGH);
}

Pcf8523SystemClock::Pcf8523SystemClock() {
    SystemClock = this;
}

bool Pcf8523SystemClock::setup() {
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
        else {
            DEBUG_PRINTLN(F("RTC Ready"));
        }
    }

    available = true;
    #endif
    #endif

    return true;
}

bool Pcf8523SystemClock::initialized() {
    return available && rtc.initialized();
}

uint32_t Pcf8523SystemClock::now() {
    if (available) {
        return rtc.now().unixtime();
    }
    return 0;
}

bool Pcf8523SystemClock::set(uint32_t newTime) {
    if (!available) {
        return false;
    }
    uint32_t before = now();
    bool uninitialized = !rtc.initialized();
    DEBUG_PRINT("Clock Adjusted before=");
    DEBUG_PRINTLN(before);
    rtc.adjust(newTime);
    adjusted = newTime;
    return uninitialized;
}


Ds1307SystemClock::Ds1307SystemClock() {
    SystemClock = this;
}

bool Ds1307SystemClock::setup() {
    if (!rtc.begin()) {
        DEBUG_PRINTLN(F("RTC Missing"));
        return false;
    }
    else {
        if (!rtc.isrunning()) {
            DEBUG_PRINTLN("RTC is NOT running!");
            rtc.adjust(DateTime(__DATE__, __TIME__));
        }
        DEBUG_PRINTLN(F("RTC Ready"));
    }
    available = true;

    return true;
}

bool Ds1307SystemClock::initialized() {
    return now() > 0;
}

uint32_t Ds1307SystemClock::now() {
    if (available) {
        return rtc.now().unixtime();
    }
    return 0;
}

bool Ds1307SystemClock::set(uint32_t newTime) {
    if (!available) {
        return false;
    }
    uint32_t before = now();
    DEBUG_PRINT("Clock Adjusted before=");
    DEBUG_PRINTLN(before);
    rtc.adjust(newTime);
    adjusted = newTime;
    return true;
}
