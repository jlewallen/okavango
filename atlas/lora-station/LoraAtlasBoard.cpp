#include <Adafruit_SleepyDog.h>

#include "LoraAtlasBoard.h"
#include "network.h"

#define LOW_POWER_SLEEP_SENSORS_END   (1000 * 60 * 10)

LoraAtlasSensorBoard::LoraAtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *serialPortExpander, SensorBoard *sensorBoard, FuelGauge *gauge) :
    AtlasSensorBoard(corePlatform, serialPortExpander, sensorBoard, gauge, false) {
}

void LoraAtlasSensorBoard::doneReadingSensors(Queue *queue, atlas_sensors_packet_t *packet) {
    tryAndSendLocalQueue(queue);

    DEBUG_PRINTLN("Beginning sleep!");

    int32_t remaining = LOW_POWER_SLEEP_SENSORS_END;
    while (remaining > 0) {
        remaining -= platformDeepSleep(true);
        Watchdog.reset();
        DEBUG_PRINTLN(remaining);
        logPrinter.flush();
    }
}

void LoraAtlasSensorBoard::tryAndSendLocalQueue(Queue *queue) {
    LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST, PIN_RFM95_RST);
    NetworkProtocolState networkProtocol(FK_IDENTITY_ATLAS, NetworkState::PingForListener, &radio, queue, NULL);

    int32_t watchdogMs = Watchdog.enable();
    DEBUG_PRINT("Watchdog enabled: ");
    DEBUG_PRINTLN(watchdogMs);

    if (radio.setup()) {
        DEBUG_PRINTLN("Enabling radio");

        DEBUG_PRINT("Queue: ");
        DEBUG_PRINTLN(queue->size());

        while (true) {
            Watchdog.reset();

            networkProtocol.tick();

            if (networkProtocol.isQueueEmpty() || networkProtocol.isNobodyListening()) {
                break;
            }

            delay(10);
        }

        radio.sleep();
    }
    else {
        DEBUG_PRINTLN("No radio available");
    }

    Watchdog.disable();
}
