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
                header.rssi =radio->lastRssi(); 
                log(&header, (fk_network_packet_t *)radio->getPacket(), radio->getPacketSize());
                radio->clear();
            }
        }
    }

private:
    void logHeader(rf95_header_t *header, fk_network_packet_t *packet, const char *kind) {
        Serial.print(header->from, HEX);
        Serial.print("->");
        Serial.print(header->to, HEX);
        Serial.print(" [");
        Serial.print(header->id, HEX);
        Serial.print(",");
        Serial.print(header->flags, HEX);
        Serial.print(",");
        Serial.print(header->rssi);
        Serial.print("] ");
        Serial.print(kind);
        Serial.print(" ");
        Serial.print(packet->name);
    }

    void log(rf95_header_t *header, fk_network_packet_t *packet, size_t packetSize) {
        Serial.println();

        switch (packet->kind) {
        case FK_PACKET_KIND_PING: {
            fk_network_ping_t ping;
            memcpy((uint8_t *)&ping, (uint8_t *)packet, sizeof(fk_network_ping_t));

            logHeader(header, &ping.fk, "PING");
            Serial.println();
            break;
        }
        case FK_PACKET_KIND_PONG: {
            fk_network_pong_t pong;
            memcpy((uint8_t *)&pong, (uint8_t *)packet, sizeof(fk_network_pong_t));

            logHeader(header, &pong.fk, "PONG");
            Serial.print("time=");
            Serial.print(pong.time);
            Serial.println();
            break;
        }
        case FK_PACKET_KIND_ACK: {
            fk_network_ack_t ack;
            memcpy((uint8_t *)&ack, (uint8_t *)packet, sizeof(fk_network_ack_t));

            logHeader(header, &ack.fk, "ACK");
            Serial.println();
            break;
        }
        case FK_PACKET_KIND_ATLAS_SENSORS: {
            atlas_sensors_packet_t atlas_sensors;
            memcpy((uint8_t *)&atlas_sensors, (uint8_t *)packet, sizeof(atlas_sensors_packet_t));

            logHeader(header, &atlas_sensors.fk, "ATLAS_SENSORS");
            Serial.print("time=");
            Serial.print(atlas_sensors.time);
            Serial.print(" battery=");
            Serial.print(atlas_sensors.battery);
            for (uint8_t i = 0; i < FK_WEATHER_STATION_PACKET_NUMBER_VALUES; ++i) {
                Serial.print(" ");
                Serial.print(String(atlas_sensors.values[i], 2));
            }
            Serial.println();
            break;
        }
        case FK_PACKET_KIND_WEATHER_STATION: {
            weather_station_packet_t weather_station;
            memcpy((uint8_t *)&weather_station, (uint8_t *)packet, sizeof(weather_station_packet_t));

            logHeader(header, &weather_station.fk, "WEATHER_STATION");
            Serial.print("time=");
            Serial.print(weather_station.time);
            Serial.print(" battery=");
            Serial.println();
            break;
        }
        case FK_PACKET_KIND_DATA_BOAT_SENSORS: {
            data_boat_packet_t data_boat;
            memcpy((uint8_t *)&data_boat, (uint8_t *)packet, sizeof(data_boat_packet_t));

            logHeader(header, &data_boat.fk, "DATA_BOAT_SENSORS");
            Serial.print(" ");
            Serial.print(String(data_boat.latitude, 2));
            Serial.print(" ");
            Serial.print(String(data_boat.longitude, 2));
            Serial.print(" ");
            Serial.print(String(data_boat.altitude, 2));
            Serial.print(" ");
            Serial.print(String(data_boat.speed, 2));
            Serial.print(" ");
            Serial.print(String(data_boat.angle, 2));
            Serial.print(" ");
            Serial.print(String(data_boat.water_temperature, 2));
            Serial.print(" ");
            Serial.print(String(data_boat.pressure, 2));
            Serial.print(" ");
            Serial.print(String(data_boat.humidity, 2));
            Serial.print(" ");
            Serial.print(String(data_boat.temperature, 2));
            Serial.print(" ");
            Serial.print(String(data_boat.conductivity, 2));
            Serial.print(" ");
            Serial.print(String(data_boat.salinity, 2));
            Serial.print(" ");
            Serial.print(String(data_boat.ph, 2));
            Serial.print(" ");
            Serial.print(String(data_boat.dissolved_oxygen, 2));
            Serial.print(" ");
            Serial.print(String(data_boat.orp, 2));
            Serial.println();
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

    Serial.println("Begin");

    // corePlatform.setup();

    Serial.println("Loop");
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

