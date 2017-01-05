#include <SPI.h>
#include <SD.h>
#include <Adafruit_SleepyDog.h>
#include <Adafruit_GPS.h>
#include <RTClib.h>
#include <DHT.h>
#include <RockBlock.h>

#include "Platforms.h"
#include "core.h"

enum NgDemoState {
    WaitingGpsFix,
    ReadingSensors,
    Transmitting,
    Sleep
};

const uint32_t STATE_MAX_GPS_FIX_TIME = 3 * 60 * 1000;
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
    void tick();

private:
    bool transmission(String message);
    bool checkGps();
};

NgDemo::NgDemo()
    : gps(&Serial1) {
}

bool NgDemo::setup() {
    Serial1.begin(9600);

    gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
    gps.sendCommand(PGCMD_ANTENNA);

    state = NgDemoState::WaitingGpsFix;
    stateChangedAt = millis();

    return true;
}

void NgDemo::tick() {
    switch (state) {
    case NgDemoState::WaitingGpsFix: {
        Watchdog.reset();
        if (checkGps()) {
            state = NgDemoState::ReadingSensors;
        }
        else if (millis() - stateChangedAt > STATE_MAX_GPS_FIX_TIME) {
            state = NgDemoState::ReadingSensors;
        }
        break;
    }
    case NgDemoState::ReadingSensors: {
        Serial.println();

        DHT dht(PIN_DHT, DHT22);
        dht.begin();
        humidity = dht.readHumidity();
        temperature = dht.readTemperature();
        batteryLevel = platformBatteryLevel();

        Serial.println(humidity);
        Serial.println(temperature);
        Serial.println(batteryLevel);

        state = NgDemoState::Transmitting;
        stateChangedAt = millis();
        break;
    }
    case NgDemoState::Transmitting: {
        uint32_t uptime = millis() / (1000 * 60);
        String message(SystemClock->now());
        message += ",";
        message += "NGD";
        message += ",";
        message += latitude;
        message += ",";
        message += longitude;
        message += ",";
        message += altitude;
        message += ",";
        message += temperature;
        message += ",";
        message += humidity;
        message += ",";
        message += batteryLevel;
        message += ",";
        message += uptime;
        transmission(message);

        state = NgDemoState::Sleep;
        stateChangedAt = millis();
        break;
    }
    case NgDemoState::Sleep: {
        Watchdog.reset();
        if (millis() - stateChangedAt > STATE_MAX_SLEEP_TIME) {
            state = NgDemoState::WaitingGpsFix;
            stateChangedAt = millis();
        }
        break;
    }
    }
}

bool NgDemo::transmission(String message) {
    Serial.print("Message: ");
    Serial.println(message.c_str());

    digitalWrite(PIN_RED_LED, HIGH);

    bool success = false;
    uint32_t started = millis();
    if (message.length() > 0) {
        RockBlock rockBlock(message);
        rockBlockSerialBegin();
        SerialType &rockBlockSerial = RockBlockSerial;
        rockBlock.setSerial(&rockBlockSerial);
        while (!rockBlock.isDone() && !rockBlock.isFailed()) {
            if (millis() - started < THIRTY_MINUTES) {
                Watchdog.reset();
            }
            rockBlock.tick();
            delay(10);
        }
        success = rockBlock.isDone();
        Serial.print("RockBlock: ");
        Serial.println(success);
    }

    digitalWrite(PIN_RED_LED, LOW);

    return success;
}

bool NgDemo::checkGps() {
    Watchdog.reset();

    if (Serial1.available()) {
        while (Serial1.available()) {
            char c = gps.read();
            Serial.print(c);
        }
    }
    if (gps.newNMEAreceived()) {
        if (gps.parse(gps.lastNMEA())) {
            if (gps.fix) {
                Serial.println("GOT NMEA");
                DateTime dateTime = DateTime(gps.year, gps.month, gps.year, gps.hour, gps.minute, gps.seconds);
                uint32_t time = dateTime.unixtime();
                SystemClock->set(time);
                latitude = gps.latitudeDegrees;
                longitude = gps.longitudeDegrees;
                altitude = gps.altitude;
                return true;
            }
        }
    }
    return false;
}

CorePlatform corePlatform;
Pcf8523SystemClock Clock;
NgDemo ngd;

void setup() {
    Watchdog.enable();

    Serial.begin(115200);

    #ifdef WAIT_FOR_SERIAL
    while (!Serial) {
        delay(100);
        if (millis() > WAIT_FOR_SERIAL) {
            break;
        }
    }
    #endif

    Serial.println("Begin");

    corePlatform.setup(PIN_SD_CS, PIN_RFM95_CS, PIN_RFM95_RST);

    Serial.println("Setting up...");

    SystemClock->setup();

    Serial.println("Clock ready...");

    ngd.setup();

    Serial.println("NGD ready... loop!");
}

void loop() {
    delay(50);

    ngd.tick();
}

// vim: set ft=cpp:
