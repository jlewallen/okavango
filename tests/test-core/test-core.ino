#include "Platforms.h"

#include "core.h"
#include "protocol.h"
#include "network.h"

#ifdef FEATHER_WING_ADALOGGER
CorePlatform corePlatform(&feather_m0_lora_adalogger_wing);
NetworkProtocolState networkProtocol(NetworkState::EnqueueFromNetwork, &corePlatform);
#else
CorePlatform corePlatform(&feather_m0_adalogger_external_lora);
NetworkProtocolState networkProtocol(NetworkState::PingForListener, &corePlatform);
#endif

void refillQueue() {
    #ifndef FEATHER_WING_ADALOGGER
    pinMode(5, INPUT);

    if (digitalRead(5)) {
        Serial.println("Refilling");

        atlas_sensors_packet_t sensors;
        memzero((uint8_t *)&sensors, sizeof(atlas_sensors_packet_t));
        sensors.fk.kind = FK_PACKET_KIND_ATLAS_SENSORS;
        sensors.time = 0;
        for (uint8_t i = 0; i < 10; ++i) {
            corePlatform.enqueue((uint8_t *)&sensors);
            sensors.time++;
        }

        corePlatform.queue()->startAtBeginning();

        DEBUG_PRINTLN("Done!");
    }
    #endif
}

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

    Serial.println("Begin");

    corePlatform.setup();
    platformPostSetup();

    refillQueue();

    Serial.println("Loop");
}

uint32_t lastRefill = 0;

void loop() {
    while (1) {
        corePlatform.tick();
        networkProtocol.tick();

        if (millis() - lastRefill > 10000) {
            Serial.println(networkProtocol.numberOfPacketsReceived());
            if (corePlatform.queue()->size() == 0) {
                refillQueue();
            }
            lastRefill = millis();
        }

        if (networkProtocol.isQueueEmpty() || networkProtocol.isNobodyListening()) {
            Serial.println("Starting over...");
            networkProtocol.startOver(PingForListener);
        }

        delay(10);
    }
}

// vim: set ft=cpp:
