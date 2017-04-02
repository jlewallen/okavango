#include <SD.h>
#include <Adafruit_SleepyDog.h>
#include "Platforms.h"
#include "core.h"
#include "LoraRadio.h"
#include "Queue.h"
#include "TransmissionStatus.h"

typedef struct rf95_header_t {
    uint8_t to;
    uint8_t from;
    uint8_t flags;
    uint8_t id;
    uint8_t rssi;
} rf95_header_t;

typedef struct packet_queue_entry_t {
    packet_queue_entry_t *next;
    rf95_header_t header;
    size_t packetSize;
    fk_network_packet_t *packet;
} packet_queue_entry_t;

class Sniffer {
private:
    packet_queue_entry_t *head;
    LoraRadio *radio;
    bool enabled;
    uint32_t lastPacket;
    bool receiving;

public:
    Sniffer(LoraRadio *radio) : radio(radio), enabled(true), head(NULL), lastPacket(0) {
    }

    void setEnabled(bool enabled) {
        this->enabled = enabled;
    }

    bool tick() {
        if (enabled) {
            radio->tick();

            if (radio->hasPacket()) {
                packet_queue_entry_t *entry = (packet_queue_entry_t *)malloc(sizeof(packet_queue_entry_t));
                memzero((uint8_t *)entry, sizeof(packet_queue_entry_t));

                entry->header.to = radio->headerTo();
                entry->header.from = radio->headerFrom();
                entry->header.flags = radio->headerFlags();
                entry->header.id = radio->headerId();
                entry->header.rssi = radio->lastRssi();
                entry->packetSize = radio->getPacketSize();
                entry->next = NULL;

                entry->packet = (fk_network_packet_t *)malloc(entry->packetSize);
                memzero((uint8_t *)entry->packet, entry->packetSize);
                memcpy((uint8_t *)entry->packet, radio->getPacket(), entry->packetSize);

                if (head == NULL) {
                    head = entry;
                }
                else {
                    packet_queue_entry_t *iter = head;
                    while (iter->next != NULL) {
                        iter = iter->next;
                    }
                    iter->next = entry;
                }

                lastPacket = millis();

                radio->clear();

                receiving = true;
            }
        }

        if (millis() - lastPacket > 2000) {
            packet_queue_entry_t *iter = head;
            while (iter != NULL) {
                packet_queue_entry_t *newNext = iter->next;
                log(&iter->header, iter->packet, iter->packetSize);
                free(iter->packet);
                free(iter);
                iter = newNext;
            }
            head = NULL;

            receiving = false;
        }

        return receiving;
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
        logPrinter.print(millis());
        logPrinter.print(" ");
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
        case FK_PACKET_KIND_SONAR_STATION: {
            atlas_sensors_packet_t atlas_sensors;
            memcpy((uint8_t *)&atlas_sensors, (uint8_t *)packet, sizeof(atlas_sensors_packet_t));

            logHeader(header, &atlas_sensors.fk, "SONAR_SENSORS");
            DEBUG_PRINT("time=");
            DEBUG_PRINT(atlas_sensors.time);
            DEBUG_PRINT(" battery=");
            DEBUG_PRINT(atlas_sensors.battery);
            for (uint8_t i = 0; i < FK_SONAR_STATION_PACKET_NUMBER_VALUES; ++i) {
                DEBUG_PRINT(" ");
                DEBUG_PRINT(String(atlas_sensors.values[i], 2));
            }
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
            for (uint8_t i = 0; i < FK_ATLAS_SENSORS_PACKET_NUMBER_VALUES; ++i) {
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
        default: {
            DEBUG_PRINT("UNKNOWN: ");
            DEBUG_PRINTLN(packet->kind);
        }
    }
};

CorePlatform corePlatform;

void setup() {
    Serial.begin(115200);

    Serial.println("Begin");
}

void loop() {
    LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST, PIN_RFM95_RST);
    Sniffer sniffer(&radio);

    if (!radio.setup()) {
        platformCatastrophe(PIN_RED_LED);
    }

    Watchdog.enable();

    while (1) {
        sniffer.tick();

        Watchdog.reset();

        delay(50);
    }
}
