#include <Adafruit_SleepyDog.h>
#include <system.h>
#include <wdt.h>
#include "Collector.h"
#include "Transmissions.h"
#include "Diagnostics.h"
#include "SelfRestart.h"
#include "network.h"
#include "CollectorNetworkCallbacks.h"
#include "Queue.h"
#include "Preflight.h"
#include "Memory.h"

#define IDLE_PERIOD_SLEEP             (8192)

#define BATTERY_WAIT_START_THRESHOLD  10.0f
#define BATTERY_WAIT_STOP_THRESHOLD   30.0f
#define BATTERY_WAIT_DYING_THRESHOLD   1.0f
#define BATTERY_WAIT_CHECK_SLEEP      (8192)
#define BATTERY_WAIT_CHECK_INTERVAL   (8192 * 8)

#define AIRWAVES_BLINK_INTERVAL       10000

#define BLINKS_BATTERY                1
#define BLINKS_IDLE                   2
#define BLINKS_AIRWAVES               3

uint32_t system_deep_sleep(uint32_t ms) {
    if (Serial) {
        delay(ms);
        return ms;
    }
    return Watchdog.sleep(ms);
}

Collector::Collector() :
    configuration(&memory, FK_SETTINGS_CONFIGURATION_FILENAME),
    radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST, PIN_RFM95_RST),
    weatherStation(&memory) {
}

void Collector::setup() {
    Wire.begin();

    pinMode(PIN_ROCK_BLOCK, OUTPUT);
    digitalWrite(PIN_ROCK_BLOCK, LOW);

    memory.setup();

    gauge.powerOn();

    delay(500);

    waitForBattery();

    corePlatform.setup(PIN_SD_CS, PIN_RFM95_CS, PIN_RFM95_RST, false);

    SystemClock->setup();

    if (corePlatform.isSdAvailable()) {
        logPrinter.open();
    }

    switch (system_get_reset_cause()) {
    case SYSTEM_RESET_CAUSE_SOFTWARE: logPrinter.println("ResetCause: Software"); break;
    case SYSTEM_RESET_CAUSE_WDT: logPrinter.println("ResetCause: WDT"); break;
    case SYSTEM_RESET_CAUSE_EXTERNAL_RESET: logPrinter.println("ResetCause: External Reset"); break;
    case SYSTEM_RESET_CAUSE_BOD33: logPrinter.println("ResetCause: BOD33"); break;
    case SYSTEM_RESET_CAUSE_BOD12: logPrinter.println("ResetCause: BOD12"); break;
    case SYSTEM_RESET_CAUSE_POR: logPrinter.println("ResetCause: PoR"); break;
    }

    logTransition("Begin");
    logPrinter.flush();

    if (!configuration.read(corePlatform.isSdAvailable())) {
        DEBUG_PRINTLN("Error reading configuration");
        logPrinter.flush();
        platformCatastrophe(PIN_RED_LED);
    }

    weatherStation.setup();

    Preflight preflight(&configuration, &weatherStation, &radio);
    preflight.check();

    DEBUG_PRINTLN("Loop");
    logPrinter.flush();

    for (uint8_t i = 0; i < 3; ++i) {
        digitalWrite(PIN_RED_LED, HIGH);
        delay(50);
        digitalWrite(PIN_RED_LED, LOW);
        delay(100);
    }
}

void Collector::waitForBattery() {
    delay(500);

    float level = gauge.stateOfCharge();
    if (level < BATTERY_WAIT_START_THRESHOLD) {
        weatherStation.off();

        float voltage = gauge.cellVoltage();
        DEBUG_PRINT("Waiting for charge: ");
        DEBUG_PRINT(level);
        DEBUG_PRINT(" ");
        DEBUG_PRINTLN(voltage);
        logPrinter.flush();

        bool markedDying = false;
        uint32_t time = 0;
        while (true) {
            float level = gauge.stateOfCharge();
            if (level > BATTERY_WAIT_STOP_THRESHOLD) {
                break;
            }

            if (!markedDying && level < BATTERY_WAIT_DYING_THRESHOLD) {
                memory.markDying(SystemClock->now());
                markedDying = true;
            }

            if (level > BATTERY_WAIT_START_THRESHOLD) {
                DEBUG_PRINT("Battery: ");
                DEBUG_PRINTLN(level);
            }
            else {
                Serial.print("Battery: ");
                Serial.println(level);
            }

            uint32_t sinceCheck = 0;
            while (sinceCheck < BATTERY_WAIT_CHECK_INTERVAL) {
                sinceCheck += system_deep_sleep(BATTERY_WAIT_CHECK_SLEEP);
                Watchdog.reset();
                platformBlinks(PIN_RED_LED, BLINKS_BATTERY);
            }
            time += sinceCheck;
        }

        DEBUG_PRINT("Done, took ");
        DEBUG_PRINTLN(time);
        logPrinter.flush();

        diagnostics.recordBatterySleep(time);
    }

    memory.markAlive(SystemClock->now());
}

bool Collector::checkWeatherStation() {
    Queue queue;

    Watchdog.reset();

    DEBUG_PRINTLN("WS: Check");
    logPrinter.flush();

    bool success = false;
    uint32_t started = millis();
    while (millis() - started < memory.intervals()->weather) {
        weatherStation.tick();

        Watchdog.reset();

        if (weatherStation.hasReading()) {
            DEBUG_PRINTLN("");

            DEBUG_PRINT("&");

            weatherStation.logReadingLocally();

            gps_fix_t *fix = weatherStation.getFix();
            if (fix->valid) {
                DEBUG_PRINT("%");
                SystemClock->set(fix->time);
            }

            DEBUG_PRINT("%");

            float *values = weatherStation.getValues();
            weather_station_packet_t packet;
            memzero((uint8_t *)&packet, sizeof(weather_station_packet_t));
            packet.fk.kind = FK_PACKET_KIND_WEATHER_STATION;
            packet.time = SystemClock->now();
            packet.battery = gauge.stateOfCharge();
            for (uint8_t i = 0; i < FK_WEATHER_STATION_PACKET_NUMBER_VALUES; ++i) {
                packet.values[i] = values[i];
            }

            DEBUG_PRINT("%");

            queue.enqueue((uint8_t *)&packet, sizeof(weather_station_packet_t));

            weatherStation.clear();
            DEBUG_PRINTLN("^");
            logPrinter.flush();

            success = true;
        }

        delay(10);
    }

    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("WS: Done");
    logPrinter.flush();

    return success;
}

void Collector::checkAirwaves() {
    Queue queue;
    NetworkProtocolState networkProtocol(NetworkState::EnqueueFromNetwork, &radio, &queue, new CollectorNetworkCallbacks());

    DEBUG_PRINTLN("AW: RR");
    logPrinter.flush();

    uint32_t started = millis();
    uint32_t last = millis();
    while (millis() - started < memory.intervals()->airwaves || !networkProtocol.isQuiet()) {
        networkProtocol.tick();

        weatherStation.ignore();

        if (millis() - last > AIRWAVES_BLINK_INTERVAL) {
            platformBlinks(PIN_RED_LED, BLINKS_AIRWAVES);
            Serial.print(".");
            last = millis();

            Watchdog.reset();
        }

        delay(10);
    }

    radio.sleep();
    delay(100);

    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("AW: Done");
    logPrinter.flush();

    DEBUG_PRINTLN("AW: Exit");
    logPrinter.flush();
}

void Collector::idlePeriod() {
    DEBUG_PRINTLN("Idle: Begin");
    logPrinter.flush();

    int32_t remaining = memory.intervals()->idle;
    while (remaining >= 0) {
        remaining -= system_deep_sleep(IDLE_PERIOD_SLEEP);
        Watchdog.reset();
        platformBlinks(PIN_RED_LED, BLINKS_IDLE);
    }

    DEBUG_PRINTLN("Idle: Done");
    logPrinter.flush();

    Watchdog.enable();
}

void Collector::logTransition(const char *name) {
    DateTime dt(SystemClock->now());

    DEBUG_PRINT("## ");

    DEBUG_PRINT(dt.unixtime());

    DEBUG_PRINT(' ');
    DEBUG_PRINT(dt.year());
    DEBUG_PRINT('/');
    DEBUG_PRINT(dt.month());
    DEBUG_PRINT('/');
    DEBUG_PRINT(dt.day());
    DEBUG_PRINT(' ');
    DEBUG_PRINT(dt.hour());
    DEBUG_PRINT(':');
    DEBUG_PRINT(dt.minute());
    DEBUG_PRINT(':');
    DEBUG_PRINT(dt.second());

    DEBUG_PRINT(" ");
    DEBUG_PRINT(weatherStation.isOn());
    DEBUG_PRINT(" ");
    DEBUG_PRINT(gauge.cellVoltage());
    DEBUG_PRINT(" ");
    DEBUG_PRINT(gauge.stateOfCharge());

    DEBUG_PRINT(" >");
    DEBUG_PRINTLN(name);
}

void Collector::tick() {
    waitForBattery();

    switch (state) {
    case CollectorState::Airwaves: {
        checkAirwaves();
        logTransition("WS");
        state = CollectorState::WeatherStation;
        break;
    }
    case CollectorState::WeatherStation: {
        checkWeatherStation();
        logTransition("ID");
        state = CollectorState::Idle;
        break;
    }
    case CollectorState::Idle: {
        idlePeriod();

        TransmissionStatus status;
        if (!status.anyTransmissionsThisHour()) {
            if (SelfRestart::isRestartNecessary()) {
                Transmissions transmissions(&corePlatform, &weatherStation, SystemClock, &configuration, &status, &gauge, &memory);
                for (uint8_t i = 0; i < 3; ++i) {
                    if (transmissions.sendStatusTransmission()) {
                        break;
                    }
                }

                memory.restarting();

                // So we can see the RB logs.
                logPrinter.flush();

                delay(1000);

                platformRestart();
            }
        }

        logTransition("TX");
        state = CollectorState::Transmission;
        break;
    }
    case CollectorState::Transmission: {
        Transmissions transmissions(&corePlatform, &weatherStation, SystemClock, &configuration, &status, &gauge, &memory);
        transmissions.handleTransmissionIfNecessary();
        logTransition("AW");
        state = CollectorState::Airwaves;
        break;
    }
    }
}

void Collector::loop() {
    while (true) {
        tick();
    }
}
