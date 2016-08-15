#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "settings.h"

#define FK_PACKET_KIND_PING                                 0x0
#define FK_PACKET_KIND_PONG                                 0x1
#define FK_PACKET_KIND_ACK                                  0x2
#define FK_PACKET_KIND_ATLAS_SENSORS                        0x3
#define FK_PACKET_KIND_WEATHER_STATION                      0x4
#define FK_PACKET_KIND_DATA_BOAT_SENSORS                    0x5

#define FK_PACKET_KIND_FORCE_TRANSMISSION                   0x6
#define FK_PACKET_KIND_RUN_DIAGNOSTICS                      0x7
#define FK_PACKET_KIND_DIAGNOSTICS_COLLECTOR                0x8

/**
 * Ideas:
 * Negotiate addresses with collector.
 * Need a role indicator eventually.
 */

#define FK_ATLAS_SENSORS_PACKET_NUMBER_VALUES               11 
#define FK_WEATHER_STATION_PACKET_NUMBER_VALUES             21

// Alignment issues? -jlewallen
typedef struct fk_network_packet_t {
    uint8_t kind;
    char name[FK_SETTINGS_NAME_LENGTH];
} network_packet_t;

typedef struct fk_network_ping_t {
    fk_network_packet_t fk;
    uint8_t batch;
} network_ping_t;

typedef struct fk_network_pong_t {
    fk_network_packet_t fk;
    uint32_t time;
} fk_network_pong_t;

typedef struct atlas_sensors_packet_t {
    fk_network_packet_t fk;
    uint32_t time;
    float battery;
    float values[FK_ATLAS_SENSORS_PACKET_NUMBER_VALUES];
} atlas_sensors_packet_t;

typedef struct weather_station_packet_t {
    fk_network_packet_t fk;
    uint32_t time;
    float battery;
    float values[FK_WEATHER_STATION_PACKET_NUMBER_VALUES];
} weather_station_packet_t;

typedef struct data_boat_packet_t {
    fk_network_packet_t fk;
    uint32_t time;
    float latitude;
    float longitude;
    float altitude;
    float speed;
    float angle;

    float water_temperature;

    float pressure;
    float humidity;
    float temperature;

    float conductivity;
    float salinity;
    float ph;
    float dissolved_oxygen;
    float orp;
} data_boat_packet_t;

typedef struct fk_network_ack_t {
    fk_network_packet_t fk;
} fk_network_ack_t;

typedef struct fk_network_force_transmission_t {
    fk_network_packet_t fk;
} fk_network_force_transmission_t;

typedef struct fk_network_run_diagnostics_t {
    fk_network_packet_t fk;
} fk_network_run_diagnostics_t;

typedef struct fk_network_diagnostics_collector_t {
    fk_network_packet_t fk;
    bool sd;
    bool weather;
    bool rockblock;
    bool fona;
} fk_network_diagnostics_collector_t;

// NOTE: Slightly smaller than the largest packet we can send.
// NO reason to keep this small as it's pretty tiny compared to the SD card
// size where it's used. It means we can have larger packets up to this size
// and not break the queue.
#define FK_QUEUE_ENTRY_SIZE                                 242

extern size_t fk_packet_get_size(fk_network_packet_t *packet);

#endif
