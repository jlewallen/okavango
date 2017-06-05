#include "Platforms.h"
#include "Preflight.h"
#include <Adafruit_SleepyDog.h>
#include <IridiumSBD.h>
#include "WatchdogCallbacks.h"
#include "Diagnostics.h"

Preflight::Preflight(Configuration *configuration, WeatherStation *weatherStation, LoraRadio *radio) :
    configuration(configuration), weatherStation(weatherStation), radio(radio) {
}

bool Preflight::check() {
    DEBUG_PRINTLN("Preflight");
    bool communications = checkCommunications();
    bool weatherStation = checkWeatherStation();

    bool radioAvailable = false;
    if (radio->setup()) {
        radio->sleep();
        radioAvailable = true;
        DEBUG_PRINTLN("preflight: LoRa good");
    }
    else {
        DEBUG_PRINTLN("preflight: LoRa failed");
    }

    diagnostics.recordPreflight(communications, weatherStation, radioAvailable);

    return communications && weatherStation && radioAvailable;
}

bool Preflight::checkCommunications() {
    uint32_t started = millis();
    if (configuration->hasRockBlock()) {
        IridiumSBD rockBlock(Serial2, -1, new WatchdogCallbacks());

        rockBlockSerialBegin();
        if (Serial) {
            rockBlock.attachConsole(Serial);
            rockBlock.attachDiags(Serial);
        }
        else {
            rockBlock.attachConsole(logPrinter);
            rockBlock.attachDiags(logPrinter);
        }
        rockBlock.setPowerProfile(0);

        bool success = rockBlock.begin(3) == ISBD_SUCCESS;
        if (!success) {
            DEBUG_PRINTLN("preflight: RockBlock failed");
            return false;
        }

        DEBUG_PRINTLN("preflight: RockBlock good");
    }
    return true;
}

bool Preflight::checkWeatherStation() {
    DEBUG_PRINTLN("preflight: Checking weather station");

    weatherStation->checkCommunications();
    weatherStation->transition(WeatherStationState::Reading);

    uint32_t started = millis();
    while (millis() - started < 60 * (uint32_t)1000) {
        weatherStation->tick();

        if (weatherStation->shouldTakeReading()) {
            weatherStation->takeReading();
        }

        Watchdog.reset();

        if (weatherStation->areCommunicationsOk()) {
            weatherStation->setup();
            DEBUG_PRINTLN("preflight: Weather station good");
            return true;
        }
    }
    weatherStation->setup();

    DEBUG_PRINTLN("preflight: Weather station failed");

    return false;
}
