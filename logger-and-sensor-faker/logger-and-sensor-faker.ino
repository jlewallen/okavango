#include <Arduino.h>
#include <Adafruit_SleepyDog.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <SD.h>
#include "Platforms.h"
#include "core.h"
#include "system.h"
#include "protocol.h"
#include "network.h"
#include "LoraRadio.h"

#define TWENTY_MINUTES  (1000 * 60 * 20)

Adafruit_INA219 ina219;
atlas_sensors_packet_t packet;

void sendFakeSensorReading() {
    Queue queue;
    LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST);
    NetworkProtocolState networkProtocol(NetworkState::PingForListener, &radio, &queue, NULL);

    logPrinter.println("Sending fake sensor reading...");

    packet.fk.kind =FK_PACKET_KIND_ATLAS_SENSORS;
    packet.time = millis();
    packet.battery = 0;
    packet.values[0]++;
    queue.enqueue((uint8_t *)&packet);
    queue.startAtBeginning();

    if (radio.setup()) {
        DEBUG_PRINTLN("Enabling radio");

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
    else {
        DEBUG_PRINTLN("No radio available");
    }

    logPrinter.println("Done");
}

void setup(void) {
    Watchdog.enable();

    Serial.begin(115200);

    uint32_t currentFrequency;

    CorePlatform corePlatform;
    corePlatform.setup();

    logPrinter.open();

    switch (system_get_reset_cause()) {
    case SYSTEM_RESET_CAUSE_SOFTWARE: logPrinter.println("ResetCause: Software"); break;
    case SYSTEM_RESET_CAUSE_WDT: logPrinter.println("ResetCause: WDT"); break;
    case SYSTEM_RESET_CAUSE_EXTERNAL_RESET: logPrinter.println("ResetCause: External Reset"); break;
    case SYSTEM_RESET_CAUSE_BOD33: logPrinter.println("ResetCause: BOD33"); break;
    case SYSTEM_RESET_CAUSE_BOD12: logPrinter.println("ResetCause: BOD12"); break;
    case SYSTEM_RESET_CAUSE_POR: logPrinter.println("ResetCause: PoR"); break;
    }

    logPrinter.flush();

    memzero((uint8_t *)&packet, sizeof(atlas_sensors_packet_t));

    delay(5000);

    ina219.begin();
    ina219.setCalibration_32V_1A();
}

void loop(void) {
    Watchdog.reset();

    File file = SD.open("LOAD.CSV", FILE_WRITE);
    if (!file) {
        logPrinter.println("Unable to open file");
        platformCatastrophe(PIN_RED_LED);
        return;
    }

    while (true) {
        sendFakeSensorReading();

        uint32_t started = millis();
        while (millis() - started < TWENTY_MINUTES) {
            float shuntVoltage = ina219.getShuntVoltage_mV();
            float busVoltage = ina219.getBusVoltage_V();
            float current = ina219.getCurrent_mA();
            float loadVoltage = busVoltage + (shuntVoltage / 1000);

            Watchdog.reset();

            Serial.print(".");

            file.print(millis());
            file.print(",");
            file.print(busVoltage);
            file.print(",");
            file.print(shuntVoltage);
            file.print(",");
            file.print(loadVoltage);
            file.print(",");
            file.print(current);
            file.println("");
            file.flush();

            delay(1000);
        }

        Serial.println("");
        logPrinter.flush();
    }
}
