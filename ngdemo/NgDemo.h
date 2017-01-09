#ifndef NG_DEMO_H_INCLUDED
#define NG_DEMO_H_INCLUDED

#include <Adafruit_SleepyDog.h>
#include <Adafruit_GPS.h>
#include <RTClib.h>
#include <DHT.h>
#include <RockBlock.h>
#include "core.h"
#include <pb_encode.h>
#include <pb_decode.h>
#include "fkcomms.pb.h"

enum NgDemoState {
    WaitingGpsFix,
    ReadingSensors,
    Transmitting,
    Sleep
};

const uint32_t STATE_MAX_GPS_FIX_TIME = 3 * 60 * 1000;
const uint32_t STATE_MAX_PREFLIGHT_GPS_FIX_TIME = 60 * 1000;
const uint32_t STATE_MAX_SLEEP_TIME = 20 * 60 * 1000;

class NgDemo {
private:
    Adafruit_GPS gps;
    NgDemoState state;
    uint32_t stateChangedAt;
    float latitude;
    float longitude;
    float altitude;
    float humidity;
    float temperature;
    float batteryLevel;

public:
    NgDemo();
    bool setup();
    bool configure();
    bool preflight();
    void failPreflight(uint8_t kind);
    void tick();

private:
    size_t encodeMessage(uint8_t *buffer, size_t bufferSize);
    bool transmission();
    bool checkGps();

public:
    bool encodeFieldsCallback(pb_ostream_t *stream, const pb_field_t *field);

};

#endif
