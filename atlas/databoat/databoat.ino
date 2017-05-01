#include <SPI.h>
#include <SD.h>
#include <Adafruit_SleepyDog.h>

#include "Platforms.h"
#include "core.h"
#include "AtlasSensorBoard.h"
#include "DataBoat.h"
#include "FuelGauge.h"

class WifiAtlasSensorBoard : public AtlasSensorBoard {
public:
    WifiAtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *serialPortExpander, SensorBoard *sensorBoard, FuelGauge *gauge);

public:
    void doneReadingSensors(Queue *queue, atlas_sensors_packet_t *packet) override;
};

WifiAtlasSensorBoard::WifiAtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *serialPortExpander, SensorBoard *sensorBoard, FuelGauge *gauge) :
    AtlasSensorBoard(corePlatform, serialPortExpander, sensorBoard, gauge, false) {
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

    int32_t remaining = LOW_POWER_SLEEP_SENSORS_END;
    while (remaining > 0) {
        remaining -= platformDeepSleep(false);
        Watchdog.reset();
        DEBUG_PRINTLN(remaining);
        logPrinter.flush();
    }
}

CorePlatform corePlatform;
FuelGauge gauge;
SerialPortExpander serialPortExpander(PORT_EXPANDER_SELECT_PIN_0, PORT_EXPANDER_SELECT_PIN_1, ConductivityConfig::OnExpanderPort4);
ParallelizedAtlasScientificSensors sensorBoard(&serialPortExpander, false);
WifiAtlasSensorBoard wifiAtlasSensorBoard(&corePlatform, &serialPortExpander, &sensorBoard, &gauge);

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

    wifiAtlasSensorBoard.setup();

    Serial.println("Loop");
}

void loop() {
    wifiAtlasSensorBoard.tick();

    delay(50);
}

// vim: set ft=cpp:
