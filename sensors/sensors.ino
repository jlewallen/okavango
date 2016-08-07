#include <SPI.h>
#include <SD.h>
#include <Ds18B20.h>
#include <Adafruit_BME280.h>

#include "Platforms.h"

#include "core.h"
#include "protocol.h"

#include "AtlasScientific.h"
#include "SerialPortExpander.h"

/**
 * A board containing primarily Atlas sensors and a few others, including water temperature.
 */
class AtlasSensorBoard {
private:
    CorePlatform *corePlatform;
    SerialPortExpander portExpander;
    AtlasScientificBoard board;
    Ds18B20 ds18b20;
    Adafruit_BME280 bme;
    atlas_sensors_packet_t packet;
    uint8_t packet_value_index = 0;

public:
    AtlasSensorBoard(CorePlatform *corePlatform);

public:
    bool tick();
    void setup();

private:
    void populatePacket();
    void logPacketLocally();

};

AtlasSensorBoard::AtlasSensorBoard(CorePlatform *corePlatform)
    : corePlatform(corePlatform), portExpander(PORT_EXPANDER_SELECT_PIN_0, PORT_EXPANDER_SELECT_PIN_1), ds18b20(PIN_DS18B20) {
    memzero((uint8_t *)&packet, sizeof(atlas_sensors_packet_t));
}

bool AtlasSensorBoard::tick() {
    board.tick();

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
            #ifdef BME280
            Serial.println("Bme");

            if (!bme.begin()) {
                float temperature = bme.readTemperature();
                float pressure = bme.readPressure();
                float humidity = bme.readHumidity();

                packet.values[packet_value_index++] = temperature;
                packet.values[packet_value_index++] = pressure;
                packet.values[packet_value_index++] = humidity;
            }
            #endif

            if (ds18b20.setup()) {
                Serial.println("Ds18B20 detected!");
                packet.values[packet_value_index++] = ds18b20.getTemperature();
            }
            else {
                Serial.println("Ds18B20 missing");
                packet.values[packet_value_index++] = 0.0f;
            }

            Serial.println("Metrics");

            packet.time = corePlatform->now();
            packet.battery = platformBatteryVoltage();
            packet.fk.kind = FK_PACKET_KIND_ATLAS_SENSORS;

            logPacketLocally();

            corePlatform->enqueue((uint8_t *)&packet);

            Serial.println("Done");

            platformLowPowerSleep(LOW_POWER_SLEEP_END);

            platformRestart();
        }
    }

    return true;
}

void AtlasSensorBoard::setup() {
    portExpander.setup();
    portExpander.select(0);

    board.setSerial(&portExpanderSerial);
    board.start();

    memset((void *)&packet, 0, sizeof(atlas_sensors_packet_t));
}

void AtlasSensorBoard::populatePacket() {
    for (uint8_t i = 0; i < board.getNumberOfValues(); ++i) {
        if (packet_value_index < FK_ATLAS_SENSORS_PACKET_NUMBER_VALUES) {
            packet.values[packet_value_index++] = board.getValues()[i];
        }
        else {
            Serial.println("Not enough room for values.");
        }
    }
}

void AtlasSensorBoard::logPacketLocally() {
    Logger *logger = corePlatform->logger();

    if (logger->opened()) {
        Serial.println("Logging");
        logger->log().print(packet.fk.kind);
        logger->log().print(",");
        logger->log().print(packet.time);
        logger->log().print(",");
        logger->log().print(packet.battery);
        for (uint8_t i = 0; i < FK_ATLAS_SENSORS_PACKET_NUMBER_VALUES; ++i) {
            logger->log().print(",");
            logger->log().print(packet.values[i]);
        }
        logger->log().println();
        logger->log().flush();
    }
}

CorePlatform corePlatform;
AtlasSensorBoard atlasSensorBoard(&corePlatform);

void setup() {
    platformLowPowerSleep(LOW_POWER_SLEEP_BEGIN);

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
    atlasSensorBoard.setup();

    platformPostSetup();

    Serial.println("Loop");
}

/*
void sendPacketAndWaitForAck() {
    if (!radio.isAvailable()) {
        Serial.println("No radio available");
        return;
    }

    Serial.println("Enabling radio");

    radio.setup();

    for (uint8_t i = 0; i < 3; ++i) {
        Serial.println("Sending");

        if (!radio.send((uint8_t *)&packet, sizeof(atlas_sensors_packet_t))) {
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
*/

void loop() {
    corePlatform.tick();
    atlasSensorBoard.tick();

    delay(50);
}

// vim: set ft=cpp:
