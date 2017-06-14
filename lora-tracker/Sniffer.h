#ifndef SNIFFER_H_INCLUDED
#define SNIFFER_H_INCLUDED

#include "protocol.h"
#include "network.h"
#include "LoraRadio.h"

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

class Sniffer : public NetworkCallbacks {
private:
    FileQueue queue;
    NetworkProtocolState networkProtocol;
    packet_queue_entry_t *head = nullptr;
    PacketHandler *handler = nullptr;
    uint32_t lastPacket = 0;
    bool receiving = 0;

public:
    Sniffer(LoraRadio *radio, PacketHandler *handler);

public:
    void tick();

public:
    virtual bool forceTransmission(NetworkProtocolState *networknetworkProtocol) override;
    virtual void handlePacket(rf95_header_t *header, fk_network_packet_t *packet, size_t packetSize) override;

};

#endif
