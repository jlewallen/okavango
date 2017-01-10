#include <Adafruit_SleepyDog.h>
#include "Platforms.h"
#include "core.h"
#include "system.h"
#include "WeatherStation.h"
#include "InitialTransmissions.h"
#include "Preflight.h"
#include "SelfRestart.h"
#include "Collector.h"

Configuration configuration(FK_SETTINGS_CONFIGURATION_FILENAME);
WeatherStation weatherStation;
CorePlatform corePlatform;
Pcf8523SystemClock Clock;
Collector collector(&weatherStation, &configuration);

void setup() {
    collector.waitForBattery();

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

    Watchdog.reset();

    corePlatform.setup(PIN_SD_CS, PIN_RFM95_CS, PIN_RFM95_RST);

    SystemClock->setup();

    logPrinter.open();

    switch (system_get_reset_cause()) {
    case SYSTEM_RESET_CAUSE_SOFTWARE: logPrinter.println("ResetCause: Software"); break;
    case SYSTEM_RESET_CAUSE_WDT: logPrinter.println("ResetCause: WDT"); break;
    case SYSTEM_RESET_CAUSE_EXTERNAL_RESET: logPrinter.println("ResetCause: External Reset"); break;
    case SYSTEM_RESET_CAUSE_BOD33: logPrinter.println("ResetCause: BOD33"); break;
    case SYSTEM_RESET_CAUSE_BOD12: logPrinter.println("ResetCause: BOD12"); break;
    case SYSTEM_RESET_CAUSE_POR: logPrinter.println("ResetCause: PoR"); break;
    }

    collector.setup();

    collector.logTransition("Begin");

    logPrinter.flush();

    if (!configuration.read()) {
        DEBUG_PRINTLN("Error reading configuration");
        logPrinter.flush();
        platformCatastrophe(PIN_RED_LED);
    }

    if (configuration.hasRockBlockAttached()) {
        pinMode(PIN_ROCK_BLOCK, OUTPUT);
        digitalWrite(PIN_ROCK_BLOCK, LOW);
    }

    weatherStation.setup();

    Preflight preflight(&configuration, &weatherStation);
    preflight.check();

    // Permanantly disabling these, they frighten me in the field.
    bool disableInitialTransmissions = SelfRestart::didWeJustRestart() || !configuration.sendInitialTransmissions();
    if (!disableInitialTransmissions) {
        initialWeatherTransmissionSent = InitialTransmissions::alreadyDone(TRANSMISSION_TYPE_WEATHER);
        initialAtlasTransmissionSent = InitialTransmissions::alreadyDone(TRANSMISSION_TYPE_ATLAS) || InitialTransmissions::alreadyDone(TRANSMISSION_TYPE_SONAR);
        initialSonarTransmissionSent = InitialTransmissions::alreadyDone(TRANSMISSION_TYPE_ATLAS) || InitialTransmissions::alreadyDone(TRANSMISSION_TYPE_SONAR);
        initialLocationTransmissionSent = InitialTransmissions::alreadyDone(TRANSMISSION_TYPE_LOCATION);
        DEBUG_PRINT("Initial transmission: weather=");
        DEBUG_PRINT(initialWeatherTransmissionSent);
        DEBUG_PRINT(" atlas=");
        DEBUG_PRINT(initialAtlasTransmissionSent);
        DEBUG_PRINT(" sonar=");
        DEBUG_PRINT(initialSonarTransmissionSent);
        DEBUG_PRINT(" location=");
        DEBUG_PRINT(initialLocationTransmissionSent);
        DEBUG_PRINTLN();
    }
    else {
        initialWeatherTransmissionSent = true;
        initialAtlasTransmissionSent = true;
        initialSonarTransmissionSent = true;
        initialLocationTransmissionSent = true;
        DEBUG_PRINTLN("Initial transmission disabled.");
    }

    DEBUG_PRINTLN("Loop");

    logPrinter.flush();
}

void loop() {
    collector.loop();
}

// vim: set ft=cpp:
