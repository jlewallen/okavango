#include "Platforms.h"

#include "core.h"
#include "protocol.h"
#include "network.h"

CorePlatform corePlatform(&feather_32u4_fona_adalogger_wing_external_lora);
NetworkProtocolState networkProtocol(NetworkState::EnqueueFromNetwork, &corePlatform);

int32_t freeRam() {
    extern int __heap_start, *__brkval; 
    int v; 
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
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

    Serial.println(F("Begin"));
    Serial.println(freeRam());

    corePlatform.setup();

    platformPostSetup();

    Serial.println(F("Loop"));
}

void loop() {
    corePlatform.tick();
    networkProtocol.tick();

    delay(10);
}

// vim: set ft=cpp:
