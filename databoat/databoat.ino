#include <SPI.h>
#include <SD.h>

#include "Platforms.h"
#include "core.h"
#include "AtlasSensorBoard.h"
#include "DataBoat.h"

class WifiAtlasSensorBoard : public AtlasSensorBoard {
public:
    WifiAtlasSensorBoard(CorePlatform *corePlatform);

public:
    void doneReadingSensors(Queue *queue, atlas_sensors_packet_t *packet) override;
};

WifiAtlasSensorBoard::WifiAtlasSensorBoard(CorePlatform *corePlatform) :
    AtlasSensorBoard(corePlatform, ConductivityConfig::OnExpanderPort4) {
}

void WifiAtlasSensorBoard::doneReadingSensors(Queue *queue, atlas_sensors_packet_t *packet) {
    DataBoat dataBoat(&Serial2, 9, packet);
    dataBoat.setup();
    while (dataBoat.tick()) {
        delay(10);
    }
}

CorePlatform corePlatform;
WifiAtlasSensorBoard wifiAtlasSensorBoard(&corePlatform);

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

    corePlatform.setup();
    wifiAtlasSensorBoard.setup();

    Serial.println("Loop");
}

void loop() {
    wifiAtlasSensorBoard.tick();

    delay(50);
}

// vim: set ft=cpp:

