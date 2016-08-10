#include "Platforms.h"
#include "core.h"
#include "protocol.h"
#include "network.h"
#include "fona.h"
#include "config.h"

typedef struct fk_collector_state_t {
    uint32_t transmit_time;
} fk_collector_state_t;

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

    platformPostSetup();

    Serial.println(F("Loop"));
}

void checkAirwaves() {
    Queue queue;
    LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST);
    NetworkProtocolState networkProtocol(NetworkState::EnqueueFromNetwork, &radio, &queue);

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

        if (millis() - started > 60 * 1000 && networkProtocol.isQuiet()) {
            break;
        }
    }

    radio.sleep();
}

void sendLatestSample() {
    Queue queue;

    if (queue.size() == 0) {
        return;
    }

    FonaChild fona(NUMBER_TO_SMS);
    Serial1.begin(4800);
    SerialType &fonaSerial = Serial1;
    fona.setSerial(&fonaSerial);
    while (!fona.isDone() && !fona.isFailed()) {
        fona.tick();
        delay(10);
    }
}

void loop() {
    checkAirwaves();
    sendLatestSample();
}

// vim: set ft=cpp:
