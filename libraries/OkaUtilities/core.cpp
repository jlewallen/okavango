#include <SD.h>
#include "core.h"
#include "protocol.h"

RtcAbstractSystemClock *SystemClock;

static void toggleChipSelectPin(uint8_t pin) {
    digitalWrite(pin, HIGH);
    delay(200);
    digitalWrite(pin, LOW);
    delay(200);
    digitalWrite(pin, HIGH);
    delay(200);
}

void CorePlatform::setup(uint8_t pinSdCs, uint8_t pinRfm95Cs, uint8_t pinRfm95Rst, bool requireSd) {
    pinMode(PIN_RED_LED, OUTPUT);
    digitalWrite(PIN_RED_LED, LOW);

    pinMode(PIN_GREEN_LED, OUTPUT);
    digitalWrite(PIN_GREEN_LED, LOW);

    pinMode(pinSdCs, OUTPUT);
    pinMode(pinRfm95Cs, OUTPUT);
    pinMode(pinRfm95Rst, OUTPUT);

    toggleChipSelectPin(pinSdCs);

    toggleChipSelectPin(pinRfm95Cs);

    if (!SD.begin(pinSdCs)) {
        sdAvailable = false;
        Serial.print("core: SD missing on ");
        Serial.println(pinSdCs);

        if (requireSd) {
            platformCatastrophe(PIN_RED_LED, PLATFORM_CATASTROPHE_FAST_BLINK);
        }
    }
    else {
        sdAvailable = true;
        Serial.println("core: SD okay.");
    }

    digitalWrite(pinRfm95Rst, HIGH);

    DEBUG_PRINT("Firmware compiled on: ");
    DEBUG_PRINT(F(__DATE__));
    DEBUG_PRINT(" ");
    DEBUG_PRINT(F(__TIME__));
    DEBUG_PRINTLN();
}

Pcf8523SystemClock::Pcf8523SystemClock() {
    SystemClock = this;
}

bool Pcf8523SystemClock::setup() {
    available = false;

    #ifdef FEATHER_WING_ADALOGGER
    #ifndef FEATHER_DISABLE_RTC
    if (!rtc.begin()) {
        DEBUG_PRINTLN("core: RTC missing");
        return false;
    }
    else {
        if (!rtc.initialized()) {
            DEBUG_PRINTLN("core: RTC uninitialized");
            rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }
        else {
            DEBUG_PRINTLN("core: RTC ready");
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
    int32_t difference = newTime - before;
    bool uninitialized = !rtc.initialized();
    DEBUG_PRINT("Clock Adjusted delta=");
    DEBUG_PRINTLN(difference);
    rtc.adjust(newTime);
    adjusted = newTime;
    return uninitialized;
}

Ds1307SystemClock::Ds1307SystemClock() {
    SystemClock = this;
}

bool Ds1307SystemClock::setup() {
    available = false;

    if (!rtc.begin()) {
        DEBUG_PRINTLN(F("RTC Missing"));
        return false;
    }
    else {
        if (!rtc.isrunning()) {
            DEBUG_PRINTLN("RTC is NOT running!");
            DEBUG_PRINTLN(__DATE__);
            DEBUG_PRINTLN(__TIME__);
            DateTime newNow(__DATE__, __TIME__);
            DEBUG_PRINTLN(newNow.unixtime());
            rtc.adjust(newNow);
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

MillisSystemClock::MillisSystemClock() {
    SystemClock = this;
}

bool MillisSystemClock::setup() {
    return true;
}

bool MillisSystemClock::initialized() {
    return true;
}

uint32_t MillisSystemClock::now() {
    return millis();
}

bool MillisSystemClock::set(uint32_t newTime) {
    return true;
}

ZeroSystemClock::ZeroSystemClock() {
    SystemClock = this;
}

bool ZeroSystemClock::setup() {
    rtc.begin();

    return true;
}

bool ZeroSystemClock::initialized() {
    return true;
}

uint32_t ZeroSystemClock::now() {
    DateTime dt(
        rtc.getYear(),
        rtc.getMonth(),
        rtc.getDay(),
        rtc.getHours(),
        rtc.getMinutes(),
        rtc.getSeconds()
    );

    return dt.unixtime();
}

bool ZeroSystemClock::set(uint32_t newTime) {
    DateTime dt(newTime);

    rtc.setYear(dt.year());
    rtc.setMonth(dt.month());
    rtc.setDay(dt.day());
    rtc.setHours(dt.hour());
    rtc.setMinutes(dt.minute());
    rtc.setSeconds(dt.second());

    return true;
}
