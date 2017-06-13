#include <Adafruit_SleepyDog.h>

#include "AtlasSensorBoard.h"

AtlasSensorBoard::AtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *portExpander, SensorBoard *board, FuelGauge *gauge) :
    corePlatform(corePlatform), board(board), portExpander(portExpander), gauge(gauge) {
}

bool AtlasSensorBoard::tick() {
    Watchdog.reset();

    board->tick();

    if (board->isDone()) {
        byte newPort = portExpander->getPort() + 1;
        portExpander->select(newPort);
        if (newPort < 4) {
            DEBUG_PRINTLN(F("Next sensor"));
            board->start();
        }
        else {
            done(board);
        }
    }

    return true;
}

void AtlasSensorBoard::setup() {
    portExpander->setup();
    portExpander->select(0);

    board->start();

    Watchdog.enable();
}
