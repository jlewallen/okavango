#include <SPI.h>
#include <SD.h>
#include <Adafruit_SleepyDog.h>

#include "Platforms.h"
#include "core.h"
#include "AtlasSensorBoard.h"
#include "LoggingAtlasSensorBoard.h"

const uint8_t PIN_SPE_ISO_SEL0 = 14;
const uint8_t PIN_SPE_ISO_SEL1 = 15;
const uint8_t PIN_SPE_SEL0 = 16;
const uint8_t PIN_SPE_SEL1 = 17;

CorePlatform corePlatform;
SingleSerialPortExpander speIsolated(PIN_SPE_ISO_SEL0, PIN_SPE_ISO_SEL1, ConductivityConfig::None, &Serial2, 4);
SingleSerialPortExpander speNormal(PIN_SPE_SEL0, PIN_SPE_SEL1, ConductivityConfig::None, &Serial1, 1);
DualSerialPortExpander serialPortExpander(&speIsolated, &speNormal);
ParallelizedAtlasScientificSensors sensorBoard(&logPrinter, &serialPortExpander, true, FK_ATLAS_SENSORS_PACKET_NUMBER_VALUES);
ZeroSystemClock Clock;
LoggingAtlasSensorBoard atlasSensorBoard(&corePlatform, &serialPortExpander, &sensorBoard, nullptr, nullptr);

void setup() {
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

    corePlatform.setup(4, PIN_RFM95_CS, PIN_RFM95_RST, true);

    serialPortExpander.setup();

    Serial.println("Loop");
}

void loop() {
    atlasSensorBoard.tick();
}

// vim: set ft=cpp:
