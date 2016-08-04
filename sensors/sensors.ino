#include <Adafruit_BME280.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_SleepyDog.h>

#include "Platforms.h"
#include "AtlasScientific.h"
#include "SerialPortExpander.h"
#include "LoraRadio.h"
#include "Ds18B20.h"
#include "Logger.h"

SerialPortExpander portExpander(PORT_EXPANDER_SELECT_PIN_0, PORT_EXPANDER_SELECT_PIN_1);
LoraRadio radio(RFM95_CS, RFM95_INT, RFM95_RST);
Logger logger(PIN_SD_CS);
AtlasScientificBoard board;
Ds18B20 ds18b20(PIN_DS18B20);
Adafruit_BME280 bme;

#define SENSORS_PACKET_NUMBER_VALUES 11

typedef struct sensors_packet_t {
    uint8_t kind;
    uint32_t time;
    float battery;
    float values[SENSORS_PACKET_NUMBER_VALUES];
} sensors_packet_t;

uint8_t valueIndex = 0;
sensors_packet_t packet;

void blink(uint8_t pin) {
    #ifndef LOW_POWER
    delay(500);
    digitalWrite(pin, HIGH);
    delay(500);
    digitalWrite(pin, LOW);
    #endif
}

void catastrophe() {
    while (true) {
        blink(PIN_RED_LED);
    }
}

void populatePacket() {
    for (uint8_t i = 0; i < board.getNumberOfValues(); ++i) {
        if (valueIndex < SENSORS_PACKET_NUMBER_VALUES) {
            packet.values[valueIndex++] = board.getValues()[i];
        }
        else {
            Serial.println("Not enough room for values.");
        }
    }
}

void lowPowerSleep(uint32_t numberOfMs) {
    if (numberOfMs > 0) {
        uint32_t slept = 0;
        while (slept < numberOfMs) {
            uint32_t before = millis();
            Watchdog.sleep();
            slept += millis() - before;
            Serial.println(numberOfMs);
            Serial.println(slept);
            blink(PIN_GREEN_LED);
        }
    }
}

void setup() {
    lowPowerSleep(LOW_POWER_SLEEP_BEGIN);

    pinMode(PIN_RED_LED, OUTPUT);
    digitalWrite(PIN_RED_LED, HIGH);

    pinMode(PIN_GREEN_LED, OUTPUT);
    digitalWrite(PIN_GREEN_LED, LOW);

    Serial.begin(115200);

    #ifdef WAIT_FOR_SERIAL
    while (!Serial) {
        delay(100);
        if (millis() > WAIT_FOR_SERIAL) {
            break;
        }
    }
    #endif

    digitalWrite(PIN_RED_LED, LOW);

    Serial.println("Begin");

    portExpander.setup();
    portExpander.select(0);

    board.setSerial(&portExpanderSerial);
    board.start();

    if (!logger.setup()) {
        catastrophe();
    }

    if (!radio.setup()) {
        catastrophe();
    }

    radio.sleep();

    if (!bme.begin()) {
        catastrophe();
    }

    platformPostSetup();

    memset((void *)&packet, 0, sizeof(sensors_packet_t));

    Serial.println("Loop");
}

void logPacketLocally() {
    if (logger.opened()) {
        Serial.println("Logging");
        logger.log().print(packet.kind);
        logger.log().print(",");
        logger.log().print(packet.time);
        logger.log().print(",");
        logger.log().print(packet.battery);
        for (uint8_t i = 0; i < SENSORS_PACKET_NUMBER_VALUES; ++i) {
            logger.log().print(",");
            logger.log().print(packet.values[i]);
        }
        logger.log().println();
        logger.log().flush();
    }
}

void sendPacketAndWaitForAck() {
    Serial.println("Enabling radio");

    radio.setup();

    for (uint8_t i = 0; i < 3; ++i) {
        Serial.println("Sending");

        if (!radio.send((uint8_t *)&packet, sizeof(sensors_packet_t))) {
            Serial.println("Unable to send!");
            delay(500);
            break;
        }

        radio.waitPacketSent();

        uint32_t before = millis();
        bool gotAck = false;
        while (millis() - before < 5000) {
            radio.tick();

            if (radio.hasPacket()) {
                gotAck = true;
                radio.clear();
                break;
            }
            delay(500);
        }
        if (gotAck) {
            break;
        }
    }

    radio.sleep();
}

void loop() {
    board.tick();
    delay(50);

    if (board.isDone()) {
        populatePacket();
        byte newPort = portExpander.getPort() + 1;
        portExpander.select(newPort);
        if (newPort < 3) {
            Serial.println("Next sensor");
            board.start();
        }
        else if (newPort == 3) {
            Serial.println("Conductivity");
            board.setSerial(&conductivitySerial);
            board.start(OPEN_CONDUCTIVITY_SERIAL_ON_START);
        }
        else {
            Serial.println("Bme");

            float temperature = bme.readTemperature();
            float pressure = bme.readPressure();
            float humidity = bme.readHumidity();

            packet.values[valueIndex++] = temperature;
            packet.values[valueIndex++] = pressure;
            packet.values[valueIndex++] = humidity;

            if (ds18b20.setup()) {
                Serial.println("Ds18B20 detected!");
                packet.values[valueIndex++] = ds18b20.getTemperature();
            }
            else {
                Serial.println("Ds18B20 missing");
                packet.values[valueIndex++] = 0.0f;
            }

            Serial.println("Metrics");

            packet.kind = 0;
            packet.time = millis();
            packet.battery = platformBatteryVoltage();

            logPacketLocally();

            sendPacketAndWaitForAck();

            Serial.println("Done");

            lowPowerSleep(LOW_POWER_SLEEP_END);

            platformRestart();
        }
    }
}

// vim: set ft=cpp:
