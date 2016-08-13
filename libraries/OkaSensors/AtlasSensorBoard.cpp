#include "AtlasSensorBoard.h"

AtlasSensorBoard::AtlasSensorBoard(CorePlatform *corePlatform, bool hasConductivity) :
    corePlatform(corePlatform), portExpander(PORT_EXPANDER_SELECT_PIN_0, PORT_EXPANDER_SELECT_PIN_1),
    hasConductivity(hasConductivity) {
    memzero((uint8_t *)&packet, sizeof(atlas_sensors_packet_t));
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
        else if (newPort == 3 && hasConductivity) {
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

                packet.values[packetValueIndex++] = temperature;
                packet.values[packetValueIndex++] = pressure;
                packet.values[packetValueIndex++] = humidity;
            }
            #endif

            #ifdef PIN_DS18B20
            Serial.println("DS18B20");
            packet.values[packetValueIndex++] = getWaterTemperature();
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

            doneReadingSensors(&queue, &packet);

            Serial.println("Done");

            platformLowPowerSleep(LOW_POWER_SLEEP_END);

            platformRestart();
        }
    }

    return true;
}

void AtlasSensorBoard::doneReadingSensors(Queue *queue, atlas_sensors_packet_t *packet) {
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
        if (packetValueIndex < FK_ATLAS_SENSORS_PACKET_NUMBER_VALUES) {
            packet.values[packetValueIndex++] = board.getValues()[i];
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

