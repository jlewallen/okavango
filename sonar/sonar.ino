#define FEATHER_WING_LORA

#include <Arduino.h>
#include <Adafruit_SleepyDog.h>
#include "core.h"
#include "Platforms.h"
#include "protocol.h"
#include "Queue.h"
#include "LoraRadio.h"
#include "network.h"
#include "Logger.h"

#define FK_SONAR_DISTANCE_FROM_GROUND_IN_METERS               (1.25f)

sonar_station_packet_t packet;
MillisSystemClock Clock;
// Ds1307SystemClock Clock;
// Pcf8523SystemClock Clock;
CorePlatform corePlatform;

void setup() {
    Serial.begin(115200);

    Serial.println("Starting...");

    #ifdef WAIT_FOR_SERIAL
    while (!Serial) {
        delay(100);
        if (millis() > WAIT_FOR_SERIAL) {
            break;
        }
    }
    #endif

    Watchdog.enable();

    Serial.println("Core...");
    corePlatform.setup(PIN_SD_CS, PIN_RFM95_CS, PIN_RFM95_RST);

    Serial.println("Clock...");
    SystemClock->setup();

    Serial.println("Log...");
    logPrinter.open();

    Serial.print("Now: ");
    Serial.println(SystemClock->now());

    Serial.println("Ready!");
}

void tryAndSendLocalQueue(Queue *queue) {
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

void logPacketLocally(sonar_station_packet_t *packet) {
    File file = Logger::open(FK_SETTINGS_SONAR_DATA_FILENAME);
    if (file) {
        #define VERBOSE_LOGGING
        #ifdef VERBOSE_LOGGING
        DEBUG_PRINT("Packet:");
        #else
        DEBUG_PRINTLN("Logging");
        #endif

        file.print(packet->fk.kind);
        file.print(",");
        file.print(packet->time);
        file.print(",");
        file.print(packet->battery);
        DEBUG_PRINT(" ");
        DEBUG_PRINT(packet->fk.kind);
        DEBUG_PRINT(" ");
        DEBUG_PRINT(packet->time);
        DEBUG_PRINT(" ");
        DEBUG_PRINT(packet->battery);
        for (uint8_t i = 0; i < FK_SONAR_STATION_PACKET_NUMBER_VALUES; ++i) {
            file.print(",");
            file.print(packet->values[i]);

            DEBUG_PRINT(" ");
            DEBUG_PRINT(packet->values[i]);
        }
        file.println();
        file.close();

        #ifdef VERBOSE_LOGGING
        DEBUG_PRINTLN("");
        #endif
    }
}

void loop() {
    Queue queue;

    memzero((void *)&packet, sizeof(sonar_station_packet_t));

    packet.time = SystemClock->now();
    packet.battery = platformBatteryVoltage();
    packet.fk.kind = FK_PACKET_KIND_SONAR_STATION;

    DEBUG_PRINTLN(packet.time);

    for (int8_t i = 0; i < FK_SONAR_STATION_PACKET_NUMBER_VALUES; ++i) {
        float value = analogRead(PIN_ULTRASONIC_SENSOR);
        float voltage = value * (3.3f / 1024.0f);
        float distance = voltage * (1000.0f / 3.2f);
        float waterLevel = FK_SONAR_DISTANCE_FROM_GROUND_IN_METERS - (distance / 100.0);

        Serial.print(voltage);
        Serial.print(" ");
        Serial.print(waterLevel);
        Serial.print(" ");
        Serial.print(distance);
        Serial.print(" ");
        Serial.println(value);

        packet.values[i] = waterLevel;

        delay(1000);

        Watchdog.reset();
    }

    logPacketLocally(&packet);

    queue.enqueue((uint8_t *)&packet, sizeof(sonar_station_packet_t));
    queue.startAtBeginning();
    tryAndSendLocalQueue(&queue);

    platformLowPowerSleep(LOW_POWER_SLEEP_SENSORS_END);

    DEBUG_PRINTLN("Bye!");
    delay(100);

    platformRestart();
}
