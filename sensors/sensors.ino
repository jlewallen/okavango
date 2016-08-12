#include <SPI.h>
#include <SD.h>
#include <Adafruit_BME280.h>
#include <OneWire.h>

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
    Adafruit_BME280 bme;
    atlas_sensors_packet_t packet;
    uint8_t packet_value_index = 0;

public:
    AtlasSensorBoard(CorePlatform *corePlatform);

public:
    bool tick();
    void setup();

private:
    float getWaterTemperature();
    void populatePacket();
    void logPacketLocally();
    void tryAndSendLocalQueue(Queue *queue);

};

AtlasSensorBoard::AtlasSensorBoard(CorePlatform *corePlatform)
    : corePlatform(corePlatform), portExpander(PORT_EXPANDER_SELECT_PIN_0, PORT_EXPANDER_SELECT_PIN_1) {
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

float AtlasSensorBoard::getWaterTemperature() {
    OneWire ds(PIN_DS18B20);

    uint8_t data[12];
    uint8_t address[8];

    if (!ds.search(address)) {
        ds.reset_search();
        return -1000;
    }

    if (OneWire::crc8(address, 7) != address[7]) {
        return -1000;
    }

    if (address[0] != 0x10 && address[0] != 0x28) {
        return -1000;
    }

    ds.reset();
    ds.select(address);
    ds.write(0x44, 1);

    uint8_t present = ds.reset();
    ds.select(address);  
    ds.write(0xbe);

    for (uint8_t i = 0; i < 9; i++) {
        data[i] = ds.read();
    }

    ds.reset_search();

    uint8_t msb = data[1];
    uint8_t lsb = data[0];

    float value = ((msb << 8) | lsb);
    return value / 16;
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

            #ifdef PIN_DS18B20
            Serial.println("DS18B20");
            packet.values[packet_value_index++] = getWaterTemperature();
            #endif

            Serial.println("Metrics");

            packet.time = SystemClock.now();
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
    File file = Logger::open(FK_SETTINGS_ATLAS_DATA_FILENAME);
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
    platformSerial2Begin(9600);

    Serial.println("Loop");
}

void loop() {
    atlasSensorBoard.tick();

    delay(50);
}

// vim: set ft=cpp:
