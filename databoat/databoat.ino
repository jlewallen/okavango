#include <SPI.h>
#include <SD.h>
#include <Adafruit_SleepyDog.h>

#include "Platforms.h"
#include "core.h"
#include "AtlasSensorBoard.h"
#include "DataBoat.h"

class WifiAtlasSensorBoard : public AtlasSensorBoard {
public:
    WifiAtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *serialPortExpander, SensorBoard *sensorBoard);

public:
    void doneReadingSensors(Queue *queue, atlas_sensors_packet_t *packet) override;
};

WifiAtlasSensorBoard::WifiAtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *serialPortExpander, SensorBoard *sensorBoard) :
    AtlasSensorBoard(corePlatform, serialPortExpander, sensorBoard) {
}

void WifiAtlasSensorBoard::doneReadingSensors(Queue *queue, atlas_sensors_packet_t *packet) {
    int32_t watchdogMs = Watchdog.enable();
    Serial.print("Watchdog enabled: ");
    Serial.println(watchdogMs);

    uint32_t started = millis();
    DataBoat dataBoat(&Serial2, 9, packet);
    dataBoat.setup();
    while (dataBoat.tick()) {
        if (millis() - started < FIVE_MINUTES) {
            Watchdog.reset();
        }
        delay(10);
    }

    Watchdog.disable();

    platformLowPowerSleep(LOW_POWER_SLEEP_DATA_BOAT_END);
}

CorePlatform corePlatform;
SerialPortExpander serialPortExpander(PORT_EXPANDER_SELECT_PIN_0, PORT_EXPANDER_SELECT_PIN_1, ConductivityConfig::OnExpanderPort4);
ParallelizedAtlasScientificSensors sensorBoard(&serialPortExpander, false);
WifiAtlasSensorBoard wifiAtlasSensorBoard(&corePlatform, &serialPortExpander, &sensorBoard);

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
    #ifdef UPLOAD_ONLY
    uint32_t started = millis();
    DataBoat dataBoat(&Serial2, 9, NULL);
    Serial.println("Setup");
    dataBoat.setup(false);
    dataBoat.upload();

    Watchdog.disable();

    while (1) {
    }
    #else
    wifiAtlasSensorBoard.setup();
    #endif

    Serial.println("Loop");
}

void loop() {
    wifiAtlasSensorBoard.tick();

    delay(50);
}

// vim: set ft=cpp:
