#ifndef NG_DEMO_H_INCLUDED
#define NG_DEMO_H_INCLUDED

#include <Adafruit_SleepyDog.h>
#include <Adafruit_GPS.h>
#include <RTClib.h>
#include <DHT.h>
#include "core.h"
#include "Config.h"
#include "Queue.h"

#define NGD_WIFI
// #define NGD_ROCKBLOCK

enum NgDemoState {
    WaitingGpsFix,
    ReadingSensors,
    Transmitting,
    Sleep
};

const uint32_t STATE_MAX_GPS_FIX_TIME = 10 * 1000;
const uint32_t STATE_MAX_PREFLIGHT_GPS_TIME = 10 * 1000;
const uint32_t STATE_MAX_SLEEP_TIME = (1 * 30 * 1000);

class NgDemo {
private:
    Queue data;
    Adafruit_GPS gps;
    Config config;
    NgDemoState state;
    size_t messageSize = 0;
    uint32_t stateChangedAt = 0;
    uint32_t batteryLoggedAt = 0;
    float latitude = 0.0f;
    float longitude = 0.0f;
    float altitude = 0.0f;
    float humidity = 0.0f;
    float temperature = 0.0f;
    float batteryLevel = 0.0f;
    float batteryVoltage = 0.0f;

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

};

#endif
