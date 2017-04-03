#include <Arduino.h>
#include <Adafruit_SleepyDog.h>
#include "core.h"
#include "FuelGauge.h"
#include "Platforms.h"
#include "protocol.h"
#include "Queue.h"
#include "LoraRadio.h"
#include "network.h"
#include "Logger.h"

#define FK_SONAR_DISTANCE_FROM_GROUND_IN_METERS               (1.25f)
#define VERBOSE_LOGGING

sonar_station_packet_t packet;
Pcf8523SystemClock Clock;
CorePlatform corePlatform;
FuelGauge gauge;

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

    Wire.begin();

    gauge.powerOn();

    Watchdog.enable();

    Serial.println("Core...");
    corePlatform.setup(PIN_SD_CS, PIN_RFM95_CS, PIN_RFM95_RST, false);

    if (corePlatform.isSdAvailable()) {
        logPrinter.open();
    }

    Serial.println("Clock...");
    SystemClock->setup();

    Serial.println("Log...");
    logPrinter.open();

    DEBUG_PRINT("Now: ");
    DEBUG_PRINTLN(SystemClock->now());

    DEBUG_PRINTLN("Ready!");
    logPrinter.flush();
}

void tryAndSendLocalQueue(Queue *queue) {
    LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST, PIN_RFM95_RST);
    NetworkProtocolState networkProtocol(FK_IDENTITY_SONAR, NetworkState::PingForListener, &radio, queue, NULL);

    int32_t watchdogMs = Watchdog.enable();
    DEBUG_PRINT("Watchdog enabled: ");
    DEBUG_PRINTLN(watchdogMs);
    logPrinter.flush();

    if (radio.setup()) {
        DEBUG_PRINTLN("Enabling radio");
        logPrinter.flush();

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

    logPrinter.flush();
    Watchdog.disable();
}

void logPacketLocally(sonar_station_packet_t *packet) {
    File file = Logger::open(FK_SETTINGS_SONAR_DATA_FILENAME);
    if (file) {
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

    logPrinter.flush();
}

void loop() {
    Queue queue;

    memzero((void *)&packet, sizeof(sonar_station_packet_t));

    packet.time = SystemClock->now();

    packet.battery = gauge.stateOfCharge();

    packet.fk.kind = FK_PACKET_KIND_SONAR_STATION;

    DEBUG_PRINTLN(packet.time);
    logPrinter.flush();

    for (int8_t i = 0; i < FK_SONAR_STATION_PACKET_NUMBER_VALUES; ++i) {
        float value = analogRead(PIN_ULTRASONIC_SENSOR);
        float voltage = value * (3.3f / 1024.0f);
        float distance = voltage * (1000.0f / 3.2f);
        float waterLevel = FK_SONAR_DISTANCE_FROM_GROUND_IN_METERS - (distance / 100.0);

        DEBUG_PRINT(voltage);
        DEBUG_PRINT(" ");
        DEBUG_PRINT(waterLevel);
        DEBUG_PRINT(" ");
        DEBUG_PRINT(distance);
        DEBUG_PRINT(" ");
        DEBUG_PRINTLN(value);
        logPrinter.flush();

        packet.values[i] = waterLevel;

        delay(1000);

        Watchdog.reset();
    }

    platformBlinks(PIN_RED_LED, 3);

    logPacketLocally(&packet);

    queue.enqueue((uint8_t *)&packet, sizeof(sonar_station_packet_t));
    queue.startAtBeginning();

    tryAndSendLocalQueue(&queue);

    DEBUG_PRINTLN("Beginning sleep!");
    logPrinter.flush();

    int32_t remaining = LOW_POWER_SLEEP_SENSORS_END;
    while (remaining > 0) {
        remaining -= platformDeepSleep(true);
        Watchdog.reset();
        DEBUG_PRINTLN(remaining);
        logPrinter.flush();
    }

    DEBUG_PRINTLN("Bye!");
    logPrinter.flush();
    delay(100);

    platformRestart();
}
