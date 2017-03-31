#include "protocol.h"

size_t fk_packet_get_size(fk_network_packet_t *packet) {
    switch (packet->kind) {
    case FK_PACKET_KIND_PING: return sizeof(fk_network_ping_t);
    case FK_PACKET_KIND_PONG: return sizeof(fk_network_pong_t);
    case FK_PACKET_KIND_ACK: return sizeof(fk_network_ack_t);
    case FK_PACKET_KIND_NACK: return sizeof(fk_network_ack_t);
    case FK_PACKET_KIND_ATLAS_SENSORS: return sizeof(atlas_sensors_packet_t);
    case FK_PACKET_KIND_WEATHER_STATION: return sizeof(weather_station_packet_t);
    case FK_PACKET_KIND_SONAR_STATION: return sizeof(sonar_station_packet_t);
    case FK_PACKET_KIND_DATA_BOAT_SENSORS: return sizeof(data_boat_packet_t);
    defaut: return FK_QUEUE_ENTRY_SIZE;
    }
}
