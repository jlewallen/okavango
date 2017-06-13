#include <Adafruit_SleepyDog.h>

#include "LegacyAtlasSensorBoard.h"

LegacyAtlasSensorBoard::LegacyAtlasSensorBoard(CorePlatform *corePlatform, SerialPortExpander *portExpander, SensorBoard *board, FuelGauge *gauge, bool continuous) :
    AtlasSensorBoard(corePlatform, portExpander, board, gauge), continuous(continuous) {
}

bool LegacyAtlasSensorBoard::tick() {
#ifdef PIN_DS18B20
    if (board->isStartingFakeReads()) {
        if (!displayedWaterTemperature) {
            DEBUG_PRINT("DS18B20 ");
            float waterTemperature = getWaterTemperature();
            DEBUG_PRINTLN(waterTemperature);
            displayedWaterTemperature = true;
        }
    }
    else {
        displayedWaterTemperature = false;
    }
#endif

    return AtlasSensorBoard::tick();
}

void LegacyAtlasSensorBoard::takeExtraReadings() {
#ifdef HAVE_BME280
    DEBUG_PRINTLN("Bme");

    if (!bme.begin()) {
        float temperature = bme.readTemperature();
        float pressure = bme.readPressure();
        float humidity = bme.readHumidity();

        packet.values[packetValueIndex++] = temperature;
        packet.values[packetValueIndex++] = pressure;
        packet.values[packetValueIndex++] = humidity;
    }
    else {
        packet.values[packetValueIndex++] = 0.0f;
        packet.values[packetValueIndex++] = 0.0f;
        packet.values[packetValueIndex++] = 0.0f;
    }
#endif

    Watchdog.reset();

#ifdef PIN_DHT22
    DHT dht(PIN_DHT22, DHT22);
    dht.begin();
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    packet.values[packetValueIndex++] = humidity;
    packet.values[packetValueIndex++] = 0.0;
    packet.values[packetValueIndex++] = temperature;

    DEBUG_PRINT("DHT22 ");
    DEBUG_PRINT(humidity);
    DEBUG_PRINT(" ");
    DEBUG_PRINTLN(temperature);
#endif

    Watchdog.reset();

#ifdef PIN_DS18B20
    DEBUG_PRINT("DS18B20 ");
    float waterTemperature = getWaterTemperature();
    packet.values[packetValueIndex++] = waterTemperature;
    DEBUG_PRINTLN(waterTemperature);
#endif

}

float LegacyAtlasSensorBoard::getWaterTemperature() {
#ifdef PIN_DS18B20
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
#else
    return 0.0f;
#endif
}

void LegacyAtlasSensorBoard::writePacket(Stream &stream, atlas_sensors_packet_t *packet) {
    stream.print(packet->fk.kind);
    stream.print(",");
    stream.print(packet->time);
    stream.print(",");
    stream.print(packet->battery);

    for (uint8_t i = 0; i < FK_ATLAS_SENSORS_PACKET_NUMBER_VALUES; ++i) {
        stream.print(",");
        stream.print(packet->values[i]);
    }

    stream.println();
}

void LegacyAtlasSensorBoard::logPacketLocally() {
    Queue queue;
    queue.enqueue((uint8_t *)&packet, sizeof(atlas_sensors_packet_t));
    queue.startAtBeginning();

    File file = Logger::open(FK_SETTINGS_ATLAS_DATA_FILENAME);
    if (file) {
        writePacket(file, &packet);
        file.close();
    }

    writePacket(Serial, &packet);
}

void LegacyAtlasSensorBoard::populatePacket(SensorBoard *board) {
    for (uint8_t i = 0; i < board->getNumberOfValues(); ++i) {
        if (packetValueIndex < FK_ATLAS_SENSORS_PACKET_NUMBER_VALUES) {
            packet.values[packetValueIndex++] = board->getValues()[i];
        }
    }
}

void LegacyAtlasSensorBoard::done(SensorBoard *board) {
    packet.time = SystemClock->now();
    packet.battery = gauge != nullptr ? gauge->stateOfCharge() : 0;
    packet.fk.kind = FK_PACKET_KIND_ATLAS_SENSORS;

    populatePacket(board);
    takeExtraReadings();
    logPacketLocally();

    Watchdog.disable();

    DEBUG_PRINTLN("Bye!");
    logPrinter.flush();
    delay(100);

    if (!continuous) {
        platformRestart();
    }
    else {
        packetValueIndex = 0;
        board->takeReading();
    }
}
