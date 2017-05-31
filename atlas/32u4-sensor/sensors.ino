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
SingleSerialPortExpander serialPortExpander(PORT_EXPANDER_SELECT_PIN_0, PORT_EXPANDER_SELECT_PIN_1, ConductivityConfig::OnSerial2, &Serial1);
ParallelizedAtlasScientificSensors sensorBoard(&serialPortExpander, false);
MillisSystemClock Clock;

class LoggingAtlasSensorBoard : public AtlasSensorBoard {
public:
    LoggingAtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *serialPortExpander, SensorBoard *sensorBoard, FuelGauge *gauge);

public:
    void doneReadingSensors(Queue *queue, atlas_sensors_packet_t *packet) override;
    void tryAndSendLocalQueue(Queue *queue);
};

LoggingAtlasSensorBoard::LoggingAtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *serialPortExpander, SensorBoard *sensorBoard, FuelGauge *gauge) :
    AtlasSensorBoard(corePlatform, serialPortExpander, sensorBoard, gauge, false) {
}

void LoggingAtlasSensorBoard::doneReadingSensors(Queue *queue, atlas_sensors_packet_t *packet) {
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

void LoggingAtlasSensorBoard::tryAndSendLocalQueue(Queue *queue) {
}

LoggingAtlasSensorBoard loggingAtlasSensorBoard(&corePlatform, &serialPortExpander, &sensorBoard, &gauge);

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

    delay(500);

    waitForBattery();

    loggingAtlasSensorBoard.setup();
    Serial1.begin(9600);

    DEBUG_PRINTLN("Loop");
}

void loop() {
    loggingAtlasSensorBoard.tick();

    delay(50);
}

// vim: set ft=cpp:
