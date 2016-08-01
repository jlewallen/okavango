#include <Adafruit_BME280.h>

#include "Platforms.h"
#include "AtlasScientific.h"
#include "SerialPortExpander.h"
#include "LoraRadio.h"

SerialPortExpander portExpander(PORT_EXPANDER_SELECT_PIN_0, PORT_EXPANDER_SELECT_PIN_1);
LoraRadio radio(RFM95_CS, RFM95_INT, RFM95_RST);
AtlasScientificBoard board;
Adafruit_BME280 bme;

void finish() {
    portExpander.select(3);

    while (true) {
        delay(1000);
    }
}

#define SENSORS_PACKET_NUMBER_VALUES 10

typedef struct sensors_packet_t {
    uint8_t kind;
    uint32_t time;
    float battery;
    float values[SENSORS_PACKET_NUMBER_VALUES];
} sensors_packet_t;

uint8_t valueIndex = 0;
sensors_packet_t packet;

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

void setup() {
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

    radio.setup();
    radio.sleep();

    if (!bme.begin()) {
        Serial.println("No BME280");
    }

    platformPostSetup();

    memset((void *)&packet, 0, sizeof(sensors_packet_t));

    Serial.println("Loop");
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

            Serial.println("Metrics");

            packet.kind = 0;
            packet.time = millis();
            packet.battery = platformBatteryVoltage();

            Serial.println("Sending");

            radio.setup();

            delay(1000);

            if (!radio.send((uint8_t *)&packet, sizeof(sensors_packet_t))) {
                Serial.println("Unable to send!");
            }

            while (!radio.isIdle()) {
                Serial.println(radio.modeName());
                delay(500);
            }

            radio.sleep();

            Serial.println("Done");

            #ifdef SINGLE_RUN
            finish();
            #else
            platformRestart();
            #endif
        }
    }
}

// vim: set ft=cpp:
