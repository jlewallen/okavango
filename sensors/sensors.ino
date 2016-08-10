#include <SPI.h>
#include <SD.h>
#include <Ds18B20.h>
#include <Adafruit_BME280.h>

#include "Platforms.h"

#include "core.h"
#include "protocol.h"
#include "network.h"
#include "Logger.h"

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
    void tryAndSendLocalQueue(Queue *queue);

};

AtlasSensorBoard::AtlasSensorBoard(CorePlatform *corePlatform)
    : corePlatform(corePlatform), portExpander(PORT_EXPANDER_SELECT_PIN_0, PORT_EXPANDER_SELECT_PIN_1), ds18b20(PIN_DS18B20) {
    memzero((uint8_t *)&packet, sizeof(atlas_sensors_packet_t));
}

void AtlasSensorBoard::tryAndSendLocalQueue(Queue *queue) {
    LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST);
    NetworkProtocolState networkProtocol(NetworkState::PingForListener, &radio, queue);

    if (radio.setup()) {
        Serial.println("Enabling radio");

        radio.setup();

        Serial.print("Queue: ");
        Serial.println(queue->size());

        while (1) {
            networkProtocol.tick();

            if (networkProtocol.isQueueEmpty() || networkProtocol.isNobodyListening()) {
                break;
            }

            delay(10);
        }

        radio.sleep();
    }
    else {
        Serial.println("No radio available");
    }
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

            #ifdef DS18B20
            if (ds18b20.setup()) {
                Serial.println("Ds18B20 detected!");
                packet.values[packet_value_index++] = ds18b20.getTemperature();
            }
            else {
                Serial.println("Ds18B20 missing");
                packet.values[packet_value_index++] = 0.0f;
            }
            #endif

            Serial.println("Metrics");

            packet.time = corePlatform->now();
            packet.battery = platformBatteryVoltage();
            packet.fk.kind = FK_PACKET_KIND_ATLAS_SENSORS;

            Serial.println(packet.time);

            logPacketLocally();

            Queue queue;
            queue.enqueue((uint8_t *)&packet);
            queue.startAtBeginning();

            tryAndSendLocalQueue(&queue);

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
    File file = Logger::open(FK_SETTINGS_DATA_FILENAME);
    if (file) {
        Serial.println("Logging");
        file.print(packet.fk.kind);
        file.print(",");
        file.print(packet.time);
        file.print(",");
        file.print(packet.battery);
        for (uint8_t i = 0; i < FK_ATLAS_SENSORS_PACKET_NUMBER_VALUES; ++i) {
            file.print(",");
            file.print(packet.values[i]);
        }
        file.println();
        file.close();
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

void loop() {
    atlasSensorBoard.tick();

    delay(50);
}

// vim: set ft=cpp:
