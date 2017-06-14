#include <SPI.h>
#include <SD.h>
#include <Adafruit_SleepyDog.h>

#include "Platforms.h"
#include "core.h"
#include "AtlasSensorBoard.h"
#include "LoggingAtlasSensorBoard.h"
#include "PacketSender.h"

const uint8_t PIN_SPE_ISO_SEL0 = 14;
const uint8_t PIN_SPE_ISO_SEL1 = 15;
const uint8_t PIN_SPE_SEL0 = 16;
const uint8_t PIN_SPE_SEL1 = 17;

class SendSinglePacketHandler : public DataBoatReadingHandler {
private:
    LoraRadio radio;

public:
    SendSinglePacketHandler() : radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST, PIN_RFM95_RST) {
    }

public:
    void handleReading(data_boat_packet_t *packet, size_t numberOfValues) override {
        SingleEntryQueue queue((uint8_t *)packet);
        PacketSender sender(&radio, &queue);

        sender.send();
    }
};

CorePlatform corePlatform;
SingleSerialPortExpander speIsolated(PIN_SPE_ISO_SEL0, PIN_SPE_ISO_SEL1, ConductivityConfig::None, &Serial2, 4);
SingleSerialPortExpander speNormal(PIN_SPE_SEL0, PIN_SPE_SEL1, ConductivityConfig::None, &Serial1, 1);
DualSerialPortExpander serialPortExpander(&speIsolated, &speNormal);
ParallelizedAtlasScientificSensors sensorBoard(&logPrinter, &serialPortExpander, true, FK_ATLAS_SENSORS_PACKET_NUMBER_VALUES);
Pcf8523SystemClock Clock;
SendSinglePacketHandler packetHandler;
LoggingAtlasSensorBoard atlasSensorBoard(&corePlatform, &serialPortExpander, &sensorBoard, nullptr, &packetHandler);

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

    corePlatform.setup(PIN_SD_CS, PIN_RFM95_CS, PIN_RFM95_RST, true);

    SystemClock->setup();

    serialPortExpander.setup();

    Serial.println("Loop");
}

void loop() {
    atlasSensorBoard.tick();
}

// vim: set ft=cpp:
