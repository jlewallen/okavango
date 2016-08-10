#include "Platforms.h"
#include "core.h"
#include "protocol.h"
#include "network.h"

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

    Serial.println(F("Begin"));

    if (radio.setup()) {
        radio.sleep();
    }

    uint32_t last = 0;
    while (1) {
        networkProtocol.tick();
        delay(10);

        if (millis() - last > 1000) {
            uint16_t size = queue.size();
            if (size != 0) {
                DEBUG_PRINTLN(size);
            }
            last = millis();
        }
    }
}

void loop() {
    checkAirwaves();
}

// vim: set ft=cpp:
