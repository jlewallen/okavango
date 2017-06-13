#include <Adafruit_SleepyDog.h>

#include "LoraAtlasSensorBoard.h"
#include "PacketSender.h"

#define LOW_POWER_SLEEP_SENSORS_END   (1000 * 60 * 10)

LoraAtlasSensorBoard::LoraAtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *serialPortExpander, SensorBoard *sensorBoard, FuelGauge *gauge) :
    LegacyAtlasSensorBoard(corePlatform, serialPortExpander, sensorBoard, gauge, false) {
}

void LoraAtlasSensorBoard::done(SensorBoard *board) {
    LegacyAtlasSensorBoard::done(board);

    tryAndSendLocalQueue();

    DEBUG_PRINTLN("Beginning sleep!");

    int32_t remaining = LOW_POWER_SLEEP_SENSORS_END;
    while (remaining > 0) {
        remaining -= platformDeepSleep(true);
        Watchdog.reset();
        DEBUG_PRINTLN(remaining);
        logPrinter.flush();
    }
}

void LoraAtlasSensorBoard::tryAndSendLocalQueue() {
    Queue queue;
    LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST, PIN_RFM95_RST);
    PacketSender sender(&radio, &queue);

    sender.send();
}
