#include <SPI.h>
#include <SD.h>
#include <Adafruit_SleepyDog.h>

#include "AtlasSensorBoard.h"
#include "FuelGauge.h"
#include "network.h"
#include "LoraRadio.h"

#define BATTERY_WAIT_START_THRESHOLD  10.0f
#define BATTERY_WAIT_STOP_THRESHOLD   30.0f
#define BATTERY_WAIT_DYING_THRESHOLD   1.0f
#define BATTERY_WAIT_CHECK_SLEEP      (8192)
#define BATTERY_WAIT_CHECK_INTERVAL   (8192 * 8)

CorePlatform corePlatform;
FuelGauge gauge;
SingleSerialPortExpander serialPortExpander(PORT_EXPANDER_SELECT_PIN_0, PORT_EXPANDER_SELECT_PIN_1, ConductivityConfig::OnSerial2);
ParallelizedAtlasScientificSensors sensorBoard(&logPrinter, &serialPortExpander, true);
Pcf8523SystemClock Clock;
LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST, PIN_RFM95_RST);

class LoraAtlasSensorBoard : public AtlasSensorBoard {
public:
    LoraAtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *serialPortExpander, SensorBoard *sensorBoard, FuelGauge *gauge);

public:
    void doneReadingSensors(Queue *queue, atlas_sensors_packet_t *packet) override;
    void tryAndSendLocalQueue(Queue *queue);
};

LoraAtlasSensorBoard::LoraAtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *serialPortExpander, SensorBoard *sensorBoard, FuelGauge *gauge) :
    AtlasSensorBoard(corePlatform, serialPortExpander, sensorBoard, gauge, true) {
}

void LoraAtlasSensorBoard::doneReadingSensors(Queue *queue, atlas_sensors_packet_t *packet) {
    tryAndSendLocalQueue(queue);
}

void LoraAtlasSensorBoard::tryAndSendLocalQueue(Queue *queue) {
    NetworkProtocolState networkProtocol(FK_IDENTITY_ATLAS, NetworkState::PingForListener, &radio, queue, NULL);

    int32_t watchdogMs = Watchdog.enable();
    DEBUG_PRINT("Watchdog enabled: ");
    DEBUG_PRINTLN(watchdogMs);

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

    Watchdog.disable();
}

LoraAtlasSensorBoard loraAtlasSensorBoard(&corePlatform, &serialPortExpander, &sensorBoard, &gauge);

void waitForBattery() {
    float level = gauge.stateOfCharge();
    Serial.print("SoC: ");
    Serial.println(level);

    if (level < BATTERY_WAIT_START_THRESHOLD) {
        float voltage = gauge.cellVoltage();
        DEBUG_PRINT("Waiting for charge: ");
        DEBUG_PRINT(level);
        DEBUG_PRINT(" ");
        DEBUG_PRINTLN(voltage);
        logPrinter.flush();

        bool markedDying = false;
        uint32_t time = 0;
        while (true) {
            float level = gauge.stateOfCharge();
            if (level > BATTERY_WAIT_STOP_THRESHOLD) {
                break;
            }

            if (level > BATTERY_WAIT_START_THRESHOLD) {
                DEBUG_PRINT("Battery: ");
                DEBUG_PRINTLN(level);
            }
            else {
                Serial.print("Battery: ");
                Serial.println(level);
            }

            uint32_t sinceCheck = 0;
            while (sinceCheck < BATTERY_WAIT_CHECK_INTERVAL) {
                sinceCheck += platformDeepSleep(true);
                Watchdog.reset();
                platformBlinks(PIN_RED_LED, 1);
            }
            time += sinceCheck;
        }

        DEBUG_PRINT("Done, took ");
        DEBUG_PRINTLN(time);
        logPrinter.flush();
    }
}

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

    corePlatform.setup(PIN_SD_CS, PIN_RFM95_CS, PIN_RFM95_RST, false);

    SystemClock->setup();

    if (corePlatform.isSdAvailable()) {
        logPrinter.open();
    }

    Wire.begin();

    gauge.powerOn();

    if (!radio.setup()) {
        DEBUG_PRINTLN("No radio");
    }

    delay(500);

    waitForBattery();

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
