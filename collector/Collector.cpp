#include <Adafruit_SleepyDog.h>
#include <system.h>
#include <wdt.h>
#include "Collector.h"
#include "Transmissions.h"
#include "Diagnostics.h"
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

#define STRINGIFY(x)                  STRINGIFYX(x)
#define STRINGIFYX(x)                 #x

Collector::Collector() :
    configuration(&memory, FK_SETTINGS_CONFIGURATION_FILENAME),
    radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST, PIN_RFM95_RST),
    weatherStation(&memory, &gauge) {
}

static void blinkSlow(int8_t pin) {
    digitalWrite(pin, HIGH);
    delay(500);
    digitalWrite(pin, LOW);
    delay(500);
}

static void blinkQuick(int8_t pin) {
    digitalWrite(pin, HIGH);
    delay(50);
    digitalWrite(pin, LOW);
    delay(100);
}

void Collector::setup() {
    blinkQuick(PIN_RED_LED);

    Serial.println("Powering on fuel gauge...");

    Wire.begin();

    gauge.powerOn();

    Serial.println("Checking battery...");

    delay(500);

    waitForBattery();

    // We turn on the RockBlock now and leave this on. I think allowing the cap
    // to discharge, especially after 6 hours of inactivity is causing problems.
    // We're only really worried about the RB keeping us from powering up which
    // we've prevented here with the above call to waitForBattery. The cap
    // should draw little power once it's charged up. This may actually save us
    // energy in the long run.
    digitalWrite(PIN_ROCK_BLOCK, HIGH);

    corePlatform.setup(PIN_SD_CS, PIN_RFM95_CS, PIN_RFM95_RST, false);

    if (corePlatform.isSdAvailable()) {
        logPrinter.open();
    }

    SystemClock->setup();

    #ifdef BUILD_COMMIT
    DEBUG_PRINT("SHA1: ");
    DEBUG_PRINTLN(STRINGIFY(BUILD_COMMIT));
    #endif

    logTransition("Begin");

    switch (system_get_reset_cause()) {
    case SYSTEM_RESET_CAUSE_SOFTWARE: DEBUG_PRINTLN("ResetCause: Software"); break;
    case SYSTEM_RESET_CAUSE_WDT: DEBUG_PRINTLN("ResetCause: WDT"); break;
    case SYSTEM_RESET_CAUSE_EXTERNAL_RESET: {
        DEBUG_PRINTLN("ResetCause: External Reset");
        sendStatus = true;
        break;
    }
    case SYSTEM_RESET_CAUSE_BOD33: DEBUG_PRINTLN("ResetCause: BOD33"); break;
    case SYSTEM_RESET_CAUSE_BOD12: DEBUG_PRINTLN("ResetCause: BOD12"); break;
    case SYSTEM_RESET_CAUSE_POR: {
        DEBUG_PRINTLN("ResetCause: PoR");
        sendStatus = true;
        break;
    }
    }

    memory.setup();

    logPrinter.flush();

    weatherStation.setup();

    preflight();

    Watchdog.reset();

    if (!configuration.read(corePlatform.isSdAvailable())) {
        DEBUG_PRINTLN("Error reading configuration");
        logPrinter.flush();
        platformCatastrophe(PIN_RED_LED);
    }
}

void Collector::preflight() {
    digitalWrite(PIN_RED_LED, HIGH);

    Preflight preflight(&configuration, &weatherStation, &radio);
    bool passed = preflight.check();
    digitalWrite(PIN_RED_LED, LOW);
    delay(500);

    if (!passed) {
        // If no RockBlock just power down, maybe the rest will do the thing
        // some good?
        if (!diagnostics.communicationsPassed) {
            digitalWrite(PIN_ROCK_BLOCK, LOW);
        }

        memory.recordPreflight(diagnostics.communicationsPassed, diagnostics.weatherStationPassed, diagnostics.loraPassed);

        for (uint8_t i = 0; i < 10; ++i) {
            blinkSlow(PIN_RED_LED);

            Watchdog.reset();
        }
    }
    else {
        for (uint8_t i = 0; i < 3; ++i) {
            blinkQuick(PIN_RED_LED);

            Watchdog.reset();
        }
    }
}

void Collector::waitForBattery() {
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
                sinceCheck += deepSleep(BATTERY_WAIT_CHECK_SLEEP);
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

void Collector::checkAirwaves() {
    Queue queue;
    NetworkProtocolState networkProtocol(FK_IDENTITY_COLLECTOR, NetworkState::EnqueueFromNetwork, &radio, &queue, new CollectorNetworkCallbacks());

    DEBUG_PRINTLN("AW: RR");
    logPrinter.flush();

    uint32_t started = platformUptime();
    uint32_t last = platformUptime();
    while (platformUptime() - started < intervalToMs(memory.intervals()->airwaves) || !networkProtocol.isQuiet()) {
        networkProtocol.tick();

        if (networkProtocol.beenRunningTooLong()) {
            DEBUG_PRINTLN("We've been running too long, why?");
            break;
        }

        weatherStation.tick();

        Watchdog.reset();

        if (platformUptime() - last > AIRWAVES_BLINK_INTERVAL) {
            platformBlinks(PIN_RED_LED, BLINKS_AIRWAVES);
            Serial.print("+");

            if (quickTransmissionCheck()) {
                DEBUG_PRINTLN("Fast track to transmission.");
                break;
            }

            last = platformUptime();
        }

        delay(10);
    }

    radio.sleep();
    delay(100);

    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("AW: Done");
    DEBUG_PRINTLN("AW: Exit");
    logPrinter.flush();
}

void Collector::idlePeriod() {
    DEBUG_PRINTLN("Idle: Begin");
    logPrinter.flush();

    Watchdog.reset();

    int32_t remaining = intervalToMs(memory.intervals()->idle);
    while (remaining >= 0) {
        remaining -= deepSleep(IDLE_PERIOD_SLEEP);
        Watchdog.reset();

        platformBlinks(PIN_RED_LED, BLINKS_IDLE);
        Serial.print("+");
        Serial.flush();

        weatherStation.tick();

        if (quickTransmissionCheck()) {
            DEBUG_PRINTLN("Fast track to transmission.");
            break;
        }
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

    logPrinter.flush();
}

void Collector::tick() {
    waitForBattery();

    switch (state) {
    case CollectorState::Airwaves: {
        checkAirwaves();
        logTransition("ID");
        state = CollectorState::Idle;
        break;
    }
    case CollectorState::Idle: {
        idlePeriod();

        TransmissionStatus status;
        if (!status.anyTransmissionsThisHour()) {
            if (platformUptime() > intervalToMs(memory.intervals()->restart)) {
                DEBUG_PRINT("Restarting: ");
                DEBUG_PRINT(platformUptime());
                DEBUG_PRINTLN(" ");

                for (uint8_t i = 0; i < 3; ++i) {
                    if (sendStatusTransmission()) {
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
        if (!Serial && sendStatus) {
            float level = gauge.stateOfCharge();
            if (level > 95) {
                sendStatusTransmission();
            }
            sendStatus = false;
        }

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

bool Collector::quickTransmissionCheck() {
    TransmissionStatus status;
    int8_t kind = status.shouldWe(memory.schedules(), true);
    return kind >= 0;
}

uint32_t Collector::deepSleep(uint32_t ms) {
    if (Serial) {
        delay(ms);
        return ms;
    }
    uint32_t time = Watchdog.sleep(ms);
    platformAdjustUptime(time);
    return time;
}

bool Collector::sendStatusTransmission() {
    Transmissions transmissions(&corePlatform, &weatherStation, SystemClock, &configuration, &status, &gauge, &memory);
    return transmissions.sendStatusTransmission();
}
