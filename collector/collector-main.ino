#include <Adafruit_SleepyDog.h>
#include "Platforms.h"
#include "core.h"
#include "system.h"
#include "WeatherStation.h"
#include "Preflight.h"
#include "SelfRestart.h"
#include "Collector.h"
#include "protocol.h"

Configuration configuration(FK_SETTINGS_CONFIGURATION_FILENAME);
WeatherStation weatherStation;
CorePlatform corePlatform;
Pcf8523SystemClock Clock;
Collector collector(&weatherStation, &configuration);

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

    collector.waitForBattery();

    Watchdog.reset();

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

    collector.setup();

    collector.logTransition("Begin");

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

    Preflight preflight(&configuration, &weatherStation);
    preflight.check();

    DEBUG_PRINTLN("Loop");

    logPrinter.flush();
}

void loop() {
    collector.loop();
}

// vim: set ft=cpp:
