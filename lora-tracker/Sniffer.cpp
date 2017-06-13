#include "Sniffer.h"

Sniffer::Sniffer(LoraRadio *radio, PacketHandler *handler) : radio(radio), handler(handler) {
}

void Sniffer::tick() {
    radio->tick();

    if (radio->hasPacket()) {
        packet_queue_entry_t *entry = (packet_queue_entry_t *)malloc(sizeof(packet_queue_entry_t));
        memset((uint8_t *)entry, 0, sizeof(packet_queue_entry_t));

        entry->header.to = radio->headerTo();
        entry->header.from = radio->headerFrom();
        entry->header.flags = radio->headerFlags();
        entry->header.id = radio->headerId();
        entry->header.rssi = radio->lastRssi();
        entry->packetSize = radio->getPacketSize();
        entry->next = NULL;

        entry->packet = (fk_network_packet_t *)malloc(entry->packetSize);
        memset((uint8_t *)entry->packet, 0, entry->packetSize);
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

    if (millis() - lastPacket > 2000) {
        packet_queue_entry_t *iter = head;
        while (iter != NULL) {
            packet_queue_entry_t *newNext = iter->next;
            handler->handle(&iter->header, iter->packet, iter->packetSize);
            free(iter->packet);
            free(iter);
            iter = newNext;
        }
        head = NULL;

        receiving = false;
    }
}
