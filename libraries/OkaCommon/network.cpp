#include "network.h"

NetworkProtocolState::NetworkProtocolState(NetworkState state, CorePlatform *platform)
    : state(state), platform(platform), delay(0), lastTick(0), lastTickNonDelayed(0), pingAgainAfterDequeue(true) {
}

void NetworkProtocolState::tick() {
    bool inDelay = false;
    if (delay > 0) {
        inDelay = millis() - lastTickNonDelayed < delay;
        if (!inDelay) {
            delay = 0;
        }
    }

    lastTick = millis();
    if (!inDelay) {
        lastTickNonDelayed = lastTick;
    }

    switch (state) {
        case NetworkState::Enqueue: {
            checkForPacket();

            break;
        }
        case NetworkState::PingForListener: {
            sendPing();

            transition(NetworkState::ListenForPong, 1000);

            break;
        }
        case NetworkState::ListenForAck: {
            checkForPacket();

            break;
        }
        case NetworkState::ListenForPong: {
            checkForPacket();

            if (!inDelay) {
                transition(NetworkState::PingForListener, 0);
            }

            break;
        }
        case NetworkState::Sleep: {
            platform->radio()->sleep();

            break;
        }
    }
}

void NetworkProtocolState::sendPing() {
    DEBUG_LOG("Ping");

    fk_network_ping_t ping;
    memzero((uint8_t *)&ping, sizeof(fk_network_ping_t));
    ping.fk.kind = FK_PACKET_KIND_PING;
    platform->radio()->send((uint8_t *)&ping, sizeof(fk_network_ping_t));
}

void NetworkProtocolState::checkForPacket() {
    platform->radio()->tick();

    if (platform->radio()->hasPacket()) {
        handle((fk_network_packet_t *)platform->radio()->getPacket());
        platform->radio()->clear();
    }
}

void NetworkProtocolState::transition(NetworkState newState, uint32_t newDelay) {
    state = newState;
    delay = newDelay;
}

void NetworkProtocolState::handle(fk_network_packet_t *packet) {
    switch (packet->kind) {
    case FK_PACKET_KIND_PING: {
        DEBUG_LOG("Pong");

        // Yep, we're here. Go ahead.
        fk_network_pong_t pong;
        memzero((uint8_t *)&pong, sizeof(fk_network_pong_t));
        pong.fk.kind = FK_PACKET_KIND_PONG;
        pong.time = 0;
        platform->radio()->send((uint8_t *)&pong, sizeof(fk_network_pong_t));
        break;
    }
    case FK_PACKET_KIND_ATLAS_SENSORS: {
        if (state == NetworkState::Enqueue) {
            // Enqueue
            platform->enqueue((uint8_t *)packet);

            // Ack
            fk_network_ack_t ack;
            memzero((uint8_t *)&ack, sizeof(fk_network_ack_t));
            ack.fk.kind = FK_PACKET_KIND_ACK;
            platform->radio()->send((uint8_t *)&ack, sizeof(fk_network_ack_t));
        }
        break;
    }
    case FK_PACKET_KIND_PONG:
    case FK_PACKET_KIND_ACK: {
        if (state == NetworkState::ListenForPong || state == NetworkState::ListenForAck) {
            uint8_t *packet = platform->dequeue();
            if (packet != NULL) {
                platform->radio()->send(packet, FK_QUEUE_ENTRY_SIZE); // This is bad. -jlewallen
                free(packet);
                packet = NULL;

                transition(NetworkState::ListenForAck, 500);
            }
            else {
                if (pingAgainAfterDequeue) {
                    transition(NetworkState::PingForListener, 250);
                }
                else {
                    transition(NetworkState::Sleep, 0);
                }
            }
        }
        break;
    }
    default: {
        break;
    }
    }
}


