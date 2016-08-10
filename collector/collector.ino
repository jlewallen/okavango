#include "Platforms.h"
#include "core.h"
#include "protocol.h"
#include "network.h"

// #include "Adafruit_FONA.h"

int32_t freeRam() {
    extern int __heap_start, *__brkval; 
    int v; 
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

#define FONA_RX  9
#define FONA_TX  8
#define FONA_RST 4
#define FONA_RI  7

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

    CorePlatform corePlatform;
    corePlatform.setup();

    platformPostSetup();

    Serial.println(F("Loop"));
}

void testFona() {
    /*
    Adafruit_FONA fona(FONA_RST);
    SoftwareSerial fonaSerial(FONA_TX, FONA_RX);
    fonaSerial.begin(4800);
    if (!fona.begin(fonaSerial)) {
        Serial.println(F("No FONA"));
        while (1);
    }
    uint8_t type = fona.type();
    Serial.println(F("FONA is OK"));
    Serial.println(type);
    */
}

void checkAirwaves() {
    Queue queue;
    LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST);
    NetworkProtocolState networkProtocol(NetworkState::EnqueueFromNetwork, &radio, &queue);

    Serial.println(F("Begin"));
    Serial.println(freeRam());

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
    testFona();
    checkAirwaves();
}

// vim: set ft=cpp:
