#include <SPI.h>
#include <SD.h>
#include <Adafruit_SleepyDog.h>

#include "AtlasSensorBoard.h"

class LoraAtlasSensorBoard : public AtlasSensorBoard {
public:
    LoraAtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *serialPortExpander, SensorBoard *sensorBoard);

public:
    void doneReadingSensors(Queue *queue, atlas_sensors_packet_t *packet) override;
    void tryAndSendLocalQueue(Queue *queue);
};

LoraAtlasSensorBoard::LoraAtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *serialPortExpander, SensorBoard *sensorBoard) :
    AtlasSensorBoard(corePlatform, serialPortExpander, sensorBoard) {
}

void LoraAtlasSensorBoard::doneReadingSensors(Queue *queue, atlas_sensors_packet_t *packet) {
    #ifdef SEND_FAKE_SONAR_PACKET_FOR_TESTING
    // Fake a sonar packet for testing.
    sonar_station_packet_t sonar_packet;
    memzero((void *)&sonar_packet, sizeof(sonar_station_packet_t));
    sonar_packet.time = SystemClock->now();
    sonar_packet.battery = platformBatteryVoltage();
    sonar_packet.fk.kind = FK_PACKET_KIND_SONAR_STATION;
    for (int8_t i = 0; i < FK_SONAR_STATION_PACKET_NUMBER_VALUES; ++i) {
        sonar_packet.values[i] = (float)i;
    }
    queue->enqueue((uint8_t *)&sonar_packet, sizeof(sonar_station_packet_t));
    queue->startAtBeginning();
    #endif

    tryAndSendLocalQueue(queue);

    int32_t remaining = LOW_POWER_SLEEP_SENSORS_END;
    while (remaining > 0) {
        remaining -= Watchdog.sleep(8192);
    }
}

void LoraAtlasSensorBoard::tryAndSendLocalQueue(Queue *queue) {
    LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST, PIN_RFM95_RST);
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

void waitForBattery() {
    // TODO: Should we have a way to turn off the weather station if it's on?
    float level = platformBatteryLevel();
    if (level > 0.15) {
        return;
    }
    DEBUG_PRINT("Waiting for more battery: ");
    DEBUG_PRINTLN(level);
    uint32_t started = millis();
    while (platformBatteryLevel() < 0.3) {
        Serial.print(platformBatteryVoltage());
        Serial.print(" ");
        Serial.print(platformBatteryLevel());
        Serial.println();
        Watchdog.reset();
        delay(5000);
    }
    DEBUG_PRINT("Done, took ");
    DEBUG_PRINTLN(millis() - started);
}

CorePlatform corePlatform;
SerialPortExpander serialPortExpander(PORT_EXPANDER_SELECT_PIN_0, PORT_EXPANDER_SELECT_PIN_1, ConductivityConfig::OnSerial2);
ParallelizedAtlasScientificSensors sensorBoard(&serialPortExpander, false);
// AtlasScientificBoard sensorBoard(&serialPortExpander, false);
LoraAtlasSensorBoard loraAtlasSensorBoard(&corePlatform, &serialPortExpander, &sensorBoard);
Pcf8523SystemClock Clock;

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

    // waitForBattery();

    corePlatform.setup(PIN_SD_CS, PIN_RFM95_CS, PIN_RFM95_RST, false);
    SystemClock->setup();

    if (corePlatform.isSdAvailable()) {
        logPrinter.open();
    }

    loraAtlasSensorBoard.setup();
    Serial1.begin(9600);
    platformSerial2Begin(9600);

    DEBUG_PRINTLN("Loop");
}

void loop() {
    loraAtlasSensorBoard.tick();

    delay(50);
}

// vim: set ft=cpp:
