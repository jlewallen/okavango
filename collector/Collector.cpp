#include <Adafruit_SleepyDog.h>

#include "Collector.h"
#include "Transmissions.h"
#include "Diagnostics.h"
#include "SelfRestart.h"
#include "network.h"
#include "CollectorNetworkCallbacks.h"
#include "Queue.h"
#include "Preflight.h"
#include "system.h"

#define IDLE_PERIOD                  (1000 * 60 * 5)
#define AIRWAVES_CHECK_TIME          (1000 * 60 * 10)
#define WEATHER_STATION_CHECK_TIME   (1000 * 10)

void Collector::setup() {
    Wire.begin();

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

    if (corePlatform.isSdAvailable()) {
        if (!configuration.read()) {
            DEBUG_PRINTLN("Error reading configuration");
            logPrinter.flush();
            platformCatastrophe(PIN_RED_LED);
        }
    }

    if (configuration.hasRockBlockAttached()) {
        pinMode(PIN_ROCK_BLOCK, OUTPUT);
        digitalWrite(PIN_ROCK_BLOCK, LOW);
    }

    weatherStation.setup();

    Preflight preflight(&configuration, &weatherStation, &radio);
    preflight.check();

    DEBUG_PRINTLN("Loop");

    logPrinter.flush();

    if (false) {
        Transmissions transmissions(&corePlatform, &weatherStation, SystemClock, &configuration, &status, &gauge);
        transmissions.sendStatusTransmission();
    }
}

void Collector::waitForBattery() {
    delay(500);

    float level = gauge.stateOfCharge();
    float voltage = gauge.cellVoltage();
    if (level > 15.0f) {
        return;
    }

    weatherStation.off();

    DEBUG_PRINT("Waiting for charge: ");
    DEBUG_PRINT(level);
    DEBUG_PRINT(" ");
    DEBUG_PRINTLN(voltage);
    logPrinter.flush();

    uint32_t started = millis();
    while (gauge.stateOfCharge() < 30.0f) {
        DEBUG_PRINT("Battery: ");
        DEBUG_PRINTLN(gauge.stateOfCharge());
        logPrinter.flush();

        uint32_t times = 60 * 1000 / 8192;
        for (uint32_t i = 0; i <= times; ++i) {
            Watchdog.sleep(8192);
            Watchdog.reset();
            platformBlinks(PIN_RED_LED, 2);
        }
    }

    DEBUG_PRINT("Done, took ");
    DEBUG_PRINTLN(millis() - started);
    logPrinter.flush();

    diagnostics.recordBatterySleep(millis() - started);
}

bool Collector::checkWeatherStation() {
    Queue queue;

    Watchdog.reset();

    DEBUG_PRINTLN("WS: Check");
    logPrinter.flush();

    bool success = false;
    uint32_t started = millis();
    while (millis() - started < WEATHER_STATION_CHECK_TIME) {
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
    while (millis() - started < AIRWAVES_CHECK_TIME || !networkProtocol.isQuiet()) {
        networkProtocol.tick();

        weatherStation.ignore();

        if (millis() - last > 5000) {
            platformBlink(PIN_RED_LED);
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

    Watchdog.disable();

    uint32_t started = millis();
    while (millis() - started < IDLE_PERIOD) {
        Watchdog.sleep(8192);
        Watchdog.reset();
        platformBlinks(PIN_RED_LED, 3);
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
                Transmissions transmissions(&corePlatform, &weatherStation, SystemClock, &configuration, &status, &gauge);
                for (uint8_t i = 0; i < 3; ++i) {
                    if (transmissions.sendStatusTransmission()) {
                        break;
                    }
                }

                // So we can see the RB logs. Saw a cutoff RB transmission, which is kind of strange.
                logPrinter.flush();

                uint32_t start = millis();
                while (millis() - start < 10 * 1000) {
                    delay(100);
                    Watchdog.reset();
                }

                platformRestart();
            }
        }

        logTransition("TX");
        state = CollectorState::Transmission;
        break;
    }
    case CollectorState::Transmission: {
        Transmissions transmissions(&corePlatform, &weatherStation, SystemClock, &configuration, &status, &gauge);
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
