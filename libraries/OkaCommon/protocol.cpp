#include "protocol.h"

fk_network_packet_t *fk_network_packet_new(size_t sz, uint8_t kind) {
    fk_network_packet_t *packet = (fk_network_packet_t *)malloc(sz);
    packet->kind = kind;
    return packet;
}

void fk_network_packet_free(fk_network_packet_t *packet) {
    free(packet);
}
