#include <SPI.h>
#include <SD.h>
#include <Adafruit_SleepyDog.h>

#include "AtlasSensorBoard.h"

class LoraAtlasSensorBoard : public AtlasSensorBoard {
public:
    LoraAtlasSensorBoard(CorePlatform *corePlatform);

public:
    void doneReadingSensors(Queue *queue, atlas_sensors_packet_t *packet) override;
    void tryAndSendLocalQueue(Queue *queue);
};

LoraAtlasSensorBoard::LoraAtlasSensorBoard(CorePlatform *corePlatform) :
    AtlasSensorBoard(corePlatform, ConductivityConfig::OnSerial2, false) {
}

void LoraAtlasSensorBoard::doneReadingSensors(Queue *queue, atlas_sensors_packet_t *packet) {
    tryAndSendLocalQueue(queue);

    platformLowPowerSleep(LOW_POWER_SLEEP_SENSORS_END);
}

void LoraAtlasSensorBoard::tryAndSendLocalQueue(Queue *queue) {
    LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST);
    NetworkProtocolState networkProtocol(NetworkState::PingForListener, &radio, queue, NULL);

    int32_t watchdogMs = Watchdog.enable();
    DEBUG_PRINT("Watchdog enabled: ");
    DEBUG_PRINTLN(watchdogMs);

    if (radio.setup()) {
        DEBUG_PRINTLN("Enabling radio");

        if (radio.setup()) {
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
    }
    else {
        DEBUG_PRINTLN("No radio available");
    }

    Watchdog.disable();
}

CorePlatform corePlatform;
LoraAtlasSensorBoard loraAtlasSensorBoard(&corePlatform);

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

    #ifdef FK_WRITE_LOG_FILE
    logPrinter.open();
    #endif

    loraAtlasSensorBoard.setup();
    platformSerial2Begin(9600);

    DEBUG_PRINTLN("Loop");
}

void loop() {
    loraAtlasSensorBoard.tick();

    delay(50);
}

// vim: set ft=cpp:
