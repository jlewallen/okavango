#include "Sniffer.h"

Sniffer::Sniffer(LoraRadio *radio, PacketHandler *handler) :
    networkProtocol(FK_IDENTITY_COLLECTOR, NetworkState::EnqueueFromNetwork, radio, &queue, this), handler(handler) {
}

bool Sniffer::forceTransmission(NetworkProtocolState *networknetworkProtocol) {
    return false;
}

void Sniffer::handlePacket(rf95_header_t *header, fk_network_packet_t *packet, size_t packetSize) {
    packet_queue_entry_t *entry = (packet_queue_entry_t *)malloc(sizeof(packet_queue_entry_t));
    memset((uint8_t *)entry, 0, sizeof(packet_queue_entry_t));

    entry->header.to = header->to;
    entry->header.from = header->from;
    entry->header.flags = header->flags;
    entry->header.id = header->id;
    entry->header.rssi = header->rssi;
    entry->packetSize = packetSize;
    entry->next = nullptr;

    entry->packet = (fk_network_packet_t *)malloc(entry->packetSize);
    memset((uint8_t *)entry->packet, 0, entry->packetSize);
    memcpy((uint8_t *)entry->packet, packet, entry->packetSize);

    if (head == nullptr) {
        head = entry;
    }
    else {
        packet_queue_entry_t *iter = head;
        while (iter->next != nullptr) {
            iter = iter->next;
        }
        iter->next = entry;
    }

    lastPacket = millis();
}

void Sniffer::tick() {
    networkProtocol.tick();

    if (networkProtocol.isQuiet()) {
        if (head != nullptr) {
            Serial.println("Dequeuing...");

            packet_queue_entry_t *iter = head;

            while (iter != nullptr) {
                packet_queue_entry_t *newNext = iter->next;
                handler->handle(&iter->header, iter->packet, iter->packetSize);
                free(iter->packet);
                free(iter);
                iter = newNext;
            }
            head = nullptr;

            Serial.println("Done dequeuing");
        }
    }
}
