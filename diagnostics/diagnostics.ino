#include <SD.h>
#include "Platforms.h"
#include "core.h"
#include "Repl.h"
#include "LoraRadio.h"

typedef struct rf95_header_t {
    uint8_t to;
    uint8_t from;
    uint8_t flags;
    uint8_t id;
    uint8_t rssi;
} rf95_header_t;

class Sniffer {
private:
    LoraRadio *radio;
    bool enabled;

public:
    Sniffer(LoraRadio *radio) : radio(radio), enabled(true) {
    }

    void setEnabled(bool enabled) {
        this->enabled = enabled;
    }

    void tick() {
        if (enabled) {
            radio->tick();

            if (radio->hasPacket()) {
                rf95_header_t header;
                header.to = radio->headerTo();
                header.from = radio->headerFrom();
                header.flags = radio->headerFlags();
                header.id = radio->headerId();
                header.rssi = radio->lastRssi();
                log(&header, (fk_network_packet_t *)radio->getPacket(), radio->getPacketSize());
                radio->clear();
            }
        }
    }

private:
    void logHeader(rf95_header_t *header, fk_network_packet_t *packet, const char *kind) {
        logPrinter.print(header->from, HEX);
        logPrinter.print("->");
        logPrinter.print(header->to, HEX);
        logPrinter.print(" [");
        logPrinter.print(header->id, HEX);
        logPrinter.print(",");
        logPrinter.print(header->flags, HEX);
        logPrinter.print(",");
        logPrinter.print(header->rssi);
        logPrinter.print("] ");
        logPrinter.print(kind);
        logPrinter.print(" ");
        logPrinter.print(packet->name);
    }

    void log(rf95_header_t *header, fk_network_packet_t *packet, size_t packetSize) {
        DEBUG_PRINTLN();

        switch (packet->kind) {
        case FK_PACKET_KIND_PING: {
            fk_network_ping_t ping;
            memcpy((uint8_t *)&ping, (uint8_t *)packet, sizeof(fk_network_ping_t));

            logHeader(header, &ping.fk, "PING");
            DEBUG_PRINTLN();
            break;
        }
        case FK_PACKET_KIND_PONG: {
            fk_network_pong_t pong;
            memcpy((uint8_t *)&pong, (uint8_t *)packet, sizeof(fk_network_pong_t));

            logHeader(header, &pong.fk, "PONG");
            DEBUG_PRINT("time=");
            DEBUG_PRINT(pong.time);
            DEBUG_PRINTLN();
            break;
        }
        case FK_PACKET_KIND_ACK: {
            fk_network_ack_t ack;
            memcpy((uint8_t *)&ack, (uint8_t *)packet, sizeof(fk_network_ack_t));

            logHeader(header, &ack.fk, "ACK");
            DEBUG_PRINTLN();
            break;
        }
        case FK_PACKET_KIND_ATLAS_SENSORS: {
            atlas_sensors_packet_t atlas_sensors;
            memcpy((uint8_t *)&atlas_sensors, (uint8_t *)packet, sizeof(atlas_sensors_packet_t));

            logHeader(header, &atlas_sensors.fk, "ATLAS_SENSORS");
            DEBUG_PRINT("time=");
            DEBUG_PRINT(atlas_sensors.time);
            DEBUG_PRINT(" battery=");
            DEBUG_PRINT(atlas_sensors.battery);
            for (uint8_t i = 0; i < FK_WEATHER_STATION_PACKET_NUMBER_VALUES; ++i) {
                DEBUG_PRINT(" ");
                DEBUG_PRINT(String(atlas_sensors.values[i], 2));
            }
            DEBUG_PRINTLN();
            break;
        }
        case FK_PACKET_KIND_WEATHER_STATION: {
            weather_station_packet_t weather_station;
            memcpy((uint8_t *)&weather_station, (uint8_t *)packet, sizeof(weather_station_packet_t));

            logHeader(header, &weather_station.fk, "WEATHER_STATION");
            DEBUG_PRINT("time=");
            DEBUG_PRINT(weather_station.time);
            DEBUG_PRINT(" battery=");
            DEBUG_PRINTLN();
            break;
        }
        case FK_PACKET_KIND_DATA_BOAT_SENSORS: {
            data_boat_packet_t data_boat;
            memcpy((uint8_t *)&data_boat, (uint8_t *)packet, sizeof(data_boat_packet_t));

            logHeader(header, &data_boat.fk, "DATA_BOAT_SENSORS");
            DEBUG_PRINT(" ");
            DEBUG_PRINT(String(data_boat.latitude, 2));
            DEBUG_PRINT(" ");
            DEBUG_PRINT(String(data_boat.longitude, 2));
            DEBUG_PRINT(" ");
            DEBUG_PRINT(String(data_boat.altitude, 2));
            DEBUG_PRINT(" ");
            DEBUG_PRINT(String(data_boat.speed, 2));
            DEBUG_PRINT(" ");
            DEBUG_PRINT(String(data_boat.angle, 2));
            DEBUG_PRINT(" ");
            DEBUG_PRINT(String(data_boat.water_temperature, 2));
            DEBUG_PRINT(" ");
            DEBUG_PRINT(String(data_boat.pressure, 2));
            DEBUG_PRINT(" ");
            DEBUG_PRINT(String(data_boat.humidity, 2));
            DEBUG_PRINT(" ");
            DEBUG_PRINT(String(data_boat.temperature, 2));
            DEBUG_PRINT(" ");
            DEBUG_PRINT(String(data_boat.conductivity, 2));
            DEBUG_PRINT(" ");
            DEBUG_PRINT(String(data_boat.salinity, 2));
            DEBUG_PRINT(" ");
            DEBUG_PRINT(String(data_boat.ph, 2));
            DEBUG_PRINT(" ");
            DEBUG_PRINT(String(data_boat.dissolved_oxygen, 2));
            DEBUG_PRINT(" ");
            DEBUG_PRINT(String(data_boat.orp, 2));
            DEBUG_PRINTLN();
            break;
        }
        }
    }
};

class DiagnosticsRepl : public Repl {
private:
    Sniffer *sniffer;
    LoraRadio *radio;

public:
    DiagnosticsRepl(Sniffer *sniffer, LoraRadio *radio) :
        sniffer(sniffer), radio(radio) {
    }

    bool doWork() {
        return false;
    }

    void handle(String command) {
        if (command == "tx") {
            fk_network_force_transmission_t  force_transmission;
            memzero((uint8_t *)&force_transmission, sizeof(fk_network_force_transmission_t));
            force_transmission.fk.kind = FK_PACKET_KIND_FORCE_TRANSMISSION;
            radio->send((uint8_t *)&force_transmission, sizeof(fk_network_force_transmission_t));
            radio->waitPacketSent();
        }
    }
};

CorePlatform corePlatform;

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

    pinMode(PIN_SD_CS, OUTPUT);
    digitalWrite(PIN_SD_CS, HIGH);

    if (!SD.begin(PIN_SD_CS)) {
        DEBUG_PRINTLN(F("SD Missing"));
        platformCatastrophe(PIN_RED_LED);
    }

    #ifdef FK_WRITE_LOG_FILE
    logPrinter.open();
    #endif

    Serial.println("Begin");

    // corePlatform.setup();

    DEBUG_PRINTLN("Loop");
}

void loop() {
    LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST);
    Sniffer sniffer(&radio);
    DiagnosticsRepl repl(&sniffer, &radio);

    if (!radio.setup()) {
        platformCatastrophe(PIN_RED_LED);
    }

    while (1) {
        repl.tick();

        sniffer.tick();

        delay(50);
    }
}

// vim: set ft=cpp:
