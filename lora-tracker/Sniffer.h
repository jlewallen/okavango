#ifndef SNIFFER_H_INCLUDED
#define SNIFFER_H_INCLUDED

#include "protocol.h"
#include "network.h"
#include "LoraRadio.h"

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

class PacketHandler {
public:
    virtual void handle(rf95_header_t *header, fk_network_packet_t *packet, size_t packetSize) = 0;

};

class Sniffer {
private:
    packet_queue_entry_t *head = nullptr;
    LoraRadio *radio = nullptr;
    PacketHandler *handler = nullptr;
    uint32_t lastPacket = 0;
    bool receiving = 0;

public:
    Sniffer(LoraRadio *radio, PacketHandler *handler);

public:
    void tick();

};

#endif
