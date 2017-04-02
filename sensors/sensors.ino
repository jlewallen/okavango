#include <SPI.h>
#include <SD.h>
#include <Adafruit_SleepyDog.h>

#include "AtlasSensorBoard.h"
#include "FuelGauge.h"

#define BATTERY_WAIT_START_THRESHOLD  10.0f
#define BATTERY_WAIT_STOP_THRESHOLD   30.0f
#define BATTERY_WAIT_DYING_THRESHOLD   1.0f
#define BATTERY_WAIT_CHECK_SLEEP      (8192)
#define BATTERY_WAIT_CHECK_INTERVAL   (8192 * 8)

CorePlatform corePlatform;
FuelGauge gauge;
SerialPortExpander serialPortExpander(PORT_EXPANDER_SELECT_PIN_0, PORT_EXPANDER_SELECT_PIN_1, ConductivityConfig::OnSerial2);
ParallelizedAtlasScientificSensors sensorBoard(&serialPortExpander, false);
Pcf8523SystemClock Clock;

static uint32_t deepSleep(uint32_t ms) {
    if (Serial) {
        delay(ms);
        return ms;
    }
    return Watchdog.sleep(ms);
}

class LoraAtlasSensorBoard : public AtlasSensorBoard {
public:
    LoraAtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *serialPortExpander, SensorBoard *sensorBoard, FuelGauge *gauge);

public:
    void doneReadingSensors(Queue *queue, atlas_sensors_packet_t *packet) override;
    void tryAndSendLocalQueue(Queue *queue);
};

LoraAtlasSensorBoard::LoraAtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *serialPortExpander, SensorBoard *sensorBoard, FuelGauge *gauge) :
    AtlasSensorBoard(corePlatform, serialPortExpander, sensorBoard, gauge) {
}

void LoraAtlasSensorBoard::doneReadingSensors(Queue *queue, atlas_sensors_packet_t *packet) {
    #ifdef SEND_FAKE_SONAR_PACKET_FOR_TESTING
    // Fake a sonar packet for testing.
    sonar_station_packet_t sonar_packet;
    memzero((void *)&sonar_packet, sizeof(sonar_station_packet_t));
    sonar_packet.time = SystemClock->now();
    sonar_packet.battery = gauge.stateOfCharge();
    sonar_packet.fk.kind = FK_PACKET_KIND_SONAR_STATION;
    for (int8_t i = 0; i < FK_SONAR_STATION_PACKET_NUMBER_VALUES; ++i) {
        sonar_packet.values[i] = (float)i;
    }
    queue->enqueue((uint8_t *)&sonar_packet, sizeof(sonar_station_packet_t));
    queue->startAtBeginning();
    #endif

    tryAndSendLocalQueue(queue);

    Serial.println("Beginning sleep!");

    int32_t remaining = LOW_POWER_SLEEP_SENSORS_END;
    while (remaining > 0) {
        remaining -= deepSleep(8192);
        Watchdog.reset();
        Serial.println(remaining);
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

// AtlasScientificBoard sensorBoard(&serialPortExpander, false);
LoraAtlasSensorBoard loraAtlasSensorBoard(&corePlatform, &serialPortExpander, &sensorBoard, &gauge);

void waitForBattery() {
    float level = gauge.stateOfCharge();
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
                sinceCheck += deepSleep(BATTERY_WAIT_CHECK_SLEEP);
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

    Wire.begin();

    gauge.powerOn();

    #ifdef WAIT_FOR_SERIAL
    while (!Serial) {
        delay(100);
        if (millis() > WAIT_FOR_SERIAL) {
            break;
        }
    }
    #endif

    Serial.println("Begin");

    waitForBattery();

    corePlatform.setup(PIN_SD_CS, PIN_RFM95_CS, PIN_RFM95_RST, false);

    if (corePlatform.isSdAvailable()) {
        logPrinter.open();
    }

    SystemClock->setup();

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
