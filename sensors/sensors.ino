#include <SPI.h>
#include <SD.h>

#include "AtlasSensorBoard.h"

class LoraAtlasSensorBoard : public AtlasSensorBoard {
public:
    LoraAtlasSensorBoard(CorePlatform *corePlatform);

public:
    void doneReadingSensors(Queue *queue, atlas_sensors_packet_t *packet) override;
    void tryAndSendLocalQueue(Queue *queue);
};

LoraAtlasSensorBoard::LoraAtlasSensorBoard(CorePlatform *corePlatform) : AtlasSensorBoard(corePlatform) {
}

void LoraAtlasSensorBoard::doneReadingSensors(Queue *queue, atlas_sensors_packet_t *packet) {
    tryAndSendLocalQueue(queue);
}

void LoraAtlasSensorBoard::tryAndSendLocalQueue(Queue *queue) {
    LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST);
    NetworkProtocolState networkProtocol(NetworkState::PingForListener, &radio, queue);

    if (radio.setup()) {
        Serial.println("Enabling radio");

        radio.setup();

        Serial.print("Queue: ");
        Serial.println(queue->size());

        while (1) {
            networkProtocol.tick();

            if (networkProtocol.isQueueEmpty() || networkProtocol.isNobodyListening()) {
                break;
            }

            delay(10);
        }

        radio.sleep();
    }
    else {
        Serial.println("No radio available");
    }
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
    loraAtlasSensorBoard.setup();
    platformSerial2Begin(9600);

    Serial.println("Loop");
}

void loop() {
    loraAtlasSensorBoard.tick();

    delay(50);
}

// vim: set ft=cpp:
