#include "network.h"

NetworkProtocolState::NetworkProtocolState(NetworkState state, CorePlatform *platform)
    : state(state), platform(platform) {
}

void NetworkProtocolState::tick() {
    switch (state) {
        case NetworkState::Enqueue: {
            checkForPacket();

            break;
        }
        case NetworkState::TryDequeue: {
            fk_network_ping_t ping;
            memzero((uint8_t *)&ping, sizeof(fk_network_ping_t));
            ping.fk.kind = FK_PACKET_KIND_PING;
            platform->radio()->send((uint8_t *)&ping, sizeof(fk_network_ping_t));

            state = NetworkState::Listen;

            break;
        }
        case NetworkState::Listen: {
            checkForPacket();

            break;
        }
        case NetworkState::Sleep: {
            platform->radio()->sleep();
            break;
        }
    }
}

void NetworkProtocolState::checkForPacket() {
    platform->radio()->tick();

    if (platform->radio()->hasPacket()) {
        handle((fk_network_packet_t *)platform->radio()->getPacket());
        platform->radio()->clear();
    }
}

void NetworkProtocolState::handle(fk_network_packet_t *packet) {
    switch (packet->kind) {
    case FK_PACKET_KIND_PING: {
        // Yep, we're here. Go ahead.
        fk_network_pong_t pong;
        memzero((uint8_t *)&pong, sizeof(fk_network_pong_t));
        pong.fk.kind = FK_PACKET_KIND_PONG;
        pong.time = 0;
        platform->radio()->send((uint8_t *)&pong, sizeof(fk_network_pong_t));
        break;
    }
    case FK_PACKET_KIND_ATLAS_SENSORS: {
        // Enqueue
        platform->enqueue((uint8_t *)packet);

        // Ack
        fk_network_ack_t ack;
        memzero((uint8_t *)&ack, sizeof(fk_network_ack_t));
        ack.fk.kind = FK_PACKET_KIND_ACK;
        platform->radio()->send((uint8_t *)&ack, sizeof(fk_network_ack_t));
        break;
    }
    case FK_PACKET_KIND_PONG: {
    }
    case FK_PACKET_KIND_ACK: {
        uint8_t *packet = platform->dequeue();
        if (packet != NULL) {
            platform->radio()->send(packet, FK_QUEUE_ENTRY_SIZE); // This is bad. -jlewallen
            free(packet);
            packet = NULL;
        }
        else {
            state = NetworkState::Sleep;
        }
        break;
    }
    default: {
        break;
    }
    }
}


