#ifndef CORE_H
#define CORE_H

#include <RTClib.h>

#ifdef ARDUINO_SAMD_FEATHER_M0
#include <RTCZero.h>
#endif

#include "Platforms.h"

class CorePlatform {
private:
    bool sdAvailable;

public:
    void setup(uint8_t pinSdCs, uint8_t pinRfm95Cs, uint8_t pinRfm95Rst, bool requireSd = true);

    bool isSdAvailable() {
        return sdAvailable;
    }

};

class RtcAbstractSystemClock {
protected:
    bool available;
    uint32_t adjusted = 0;

public:
    virtual bool setup() = 0;
    virtual bool initialized() = 0;
    virtual uint32_t now() = 0;
    virtual bool set(uint32_t now) = 0;
};

class Pcf8523SystemClock : RtcAbstractSystemClock {
private:
    RTC_PCF8523 rtc;

public:
    Pcf8523SystemClock();
    virtual bool setup() override;
    virtual bool initialized() override;
    virtual uint32_t now() override;
    virtual bool set(uint32_t now) override;
};

class Ds1307SystemClock : RtcAbstractSystemClock {
private:
    RTC_DS1307 rtc;

public:
    Ds1307SystemClock();
    virtual bool setup() override;
    virtual bool initialized() override;
    virtual uint32_t now() override;
    virtual bool set(uint32_t now) override;
};

class MillisSystemClock : RtcAbstractSystemClock {
public:
    MillisSystemClock();
    virtual bool setup() override;
    virtual bool initialized() override;
    virtual uint32_t now() override;
    virtual bool set(uint32_t now) override;
};

#ifdef ARDUINO_SAMD_FEATHER_M0
class ZeroSystemClock : RtcAbstractSystemClock {
private:
    RTCZero rtc;

public:
    ZeroSystemClock();
    virtual bool setup() override;
    virtual bool initialized() override;
    virtual uint32_t now() override;
    virtual bool set(uint32_t now) override;
};
#endif

extern RtcAbstractSystemClock *SystemClock;

#endif
