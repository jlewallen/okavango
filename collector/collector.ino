#include "Platforms.h"
#include "core.h"
#include "protocol.h"
#include "network.h"
#include "fona.h"
#include "RockBlock.h"
#include "config.h"
#include "TransmissionStatus.h"
#include "WeatherStation.h"

bool radioSetup = false;

void setup() {
    platformLowPowerSleep(LOW_POWER_SLEEP_BEGIN);

    Serial.begin(115200);

    #ifdef WAIT_FOR_SERIAL
    while (!Serial) {
        delay(100);
        if (millis() > WAIT_FOR_SERIAL) {
            break;
        }
    }
    #endif

    Serial.println(F("Begin"));

    CorePlatform corePlatform;
    corePlatform.setup();

    WeatherStation weatherStation;
    weatherStation.setup();

    platformPostSetup();

    Serial.println(F("Loop"));
}

void checkAirwaves() {
    Queue queue;
    LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST);
    NetworkProtocolState networkProtocol(NetworkState::EnqueueFromNetwork, &radio, &queue);
    WeatherStation weatherStation;

    Serial.println("Checking airwaves...");

    // Can't call this more than 3 times or so because we use up all the IRQs and
    // so this would be nice to have a kind of memory?
    if (!radioSetup) {
        if (radio.setup()) {
            radio.sleep();
        }
        radioSetup = true;
    }

    uint32_t started = millis();
    uint32_t last = 0;
    while (true) {
        networkProtocol.tick();
        delay(10);

        if (millis() - last > 5000) {
            Serial.print(".");
            last = millis();
        }

        if (millis() - started > 2 * 60 * 1000 && networkProtocol.isQuiet()) {
            break;
        }

        if (weatherStation.tick()) {
            weatherStation.logReadingLocally();

            float *values = weatherStation.getValues();
            SystemClock.set((uint32_t)values[0]);

            weather_station_packet_t packet;
            memzero((uint8_t *)&packet, sizeof(weather_station_packet_t));
            packet.fk.kind = FK_PACKET_KIND_WEATHER_STATION;
            packet.time = SystemClock.now();
            packet.battery = 0.0f;
            for (uint8_t i = 0; i < FK_WEATHER_STATION_PACKET_NUMBER_VALUES; ++i) {
                packet.values[i] = values[i];
            }

            queue.enqueue((uint8_t *)&packet);

            weatherStation.clear();
        }
    }

    radio.sleep();

    Serial.println();
}

String buildTransmission(atlas_sensors_packet_t *atlas) {
    return "";
}

void handleTransmissionIfNecessary() {
    Queue queue;

    if (queue.size() == 0) {
        DEBUG_PRINTLN("Queue empty");
        return;
    }

    TransmissionStatus status;
    if (status.shouldWe()) {
        #ifdef FONA
        FonaChild fona(NUMBER_TO_SMS, "");
        Serial1.begin(4800);
        SerialType &fonaSerial = Serial1;
        fona.setSerial(&fonaSerial);
        while (!fona.isDone() && !fona.isFailed()) {
            fona.tick();
            delay(10);
        }
        #else
        RockBlock rockBlock("");
        Serial1.begin(19200);
        SerialType &rockBlockSerial = Serial1;
        rockBlock.setSerial(&rockBlockSerial);
        while (!rockBlock.isDone() && !rockBlock.isFailed()) {
            rockBlock.tick();
            delay(10);
        }
        #endif
    }
}

void loop() {
    checkAirwaves();
    handleTransmissionIfNecessary();
}

// vim: set ft=cpp:
