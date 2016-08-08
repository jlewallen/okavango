#include "Platforms.h"

#include "core.h"
#include "protocol.h"
#include "network.h"

CorePlatform corePlatform;

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

    Serial.println("Loop");
}

uint32_t lastPing = 0;

NetworkProtocolState networkProtocol(NetworkState::PingForListener, &corePlatform);
// NetworkProtocolState networkProtocol(NetworkState::Enqueue, &corePlatform);

void loop() {
    corePlatform.tick();
    networkProtocol.tick();

    delay(10);
}

// vim: set ft=cpp:
