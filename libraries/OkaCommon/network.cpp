#include "network.h"
#include "core.h"
#include "Diagnostics.h"

#define RETRY_DELAY         2500

NetworkProtocolState::NetworkProtocolState(NetworkState state, LoraRadio *radio, Queue *queue, NetworkCallbacks *networkCallbacks) :
    state(state), radio(radio), queue(queue), stateDelay(0), lastTick(0), lastTickNonDelayed(0),
    pingAgainAfterDequeue(true), packetsReceived(0), lastPacketTime(0),
    networkCallbacks(networkCallbacks) {
}

void NetworkProtocolState::tick() {
    /**
     * Some caveats to this:
     * 1) lastTickNonDelayed should be set on transition or the order here should
     *    be changed. So we would transition to a "delayed" state but the last
     *    time we recorded the lastTickNonDelayed was long past and so we would
          immediately leave it.
     * 2) Avoid more than one transition per tick.
     */
    bool wasDelayed = false;
    bool inDelay = false;
    if (stateDelay > 0) {
        wasDelayed = true;
        inDelay = millis() - lastTickNonDelayed < stateDelay;
        if (!inDelay) {
            stateDelay = 0;
        }
    }

    lastTick = millis();
    if (!inDelay) {
        lastTickNonDelayed = lastTick;
    }

    switch (state) {
        case NetworkState::EnqueueFromNetwork: {
            checkForPacket();

            if (millis() - lastPacketTime > 10000) {
                transition(NetworkState::Quiet, 500);
            }

            break;
        }
        case NetworkState::Quiet: {
            transition(NetworkState::EnqueueFromNetwork, 0);

            break;
        }
        case NetworkState::PingForListener: {
            sendPing();

            transition(NetworkState::ListenForPong, RETRY_DELAY);

            break;
        }
        case NetworkState::ListenForAck: {
            if (!inDelay) {
                if (radio->resend()) {
                    DEBUG_PRINTLN(F("Nack, rx"));
                    transition(NetworkState::ListenForAck, RETRY_DELAY);
                }
                else {
                    DEBUG_PRINTLN(F("Nack, break"));
                    transition(NetworkState::GiveListenerABreak, RETRY_DELAY);
                }
            }
            else {
                checkForPacket();
            }
            break;
        }
        case NetworkState::ListenForPong: {
            if (!inDelay) {
                if (radio->resend()) {
                    DEBUG_PRINTLN(F("Ping retry"));
                    transition(NetworkState::ListenForPong, RETRY_DELAY);
                }
                else {
                    DEBUG_PRINTLN(F("No pong"));
                    radio->sleep();
                    transition(NetworkState::NobodyListening, 0);
                }
            }
            else {
                checkForPacket();
            }
            break;
        }
        case NetworkState::GiveListenerABreak: {
            if (!inDelay) {
                transition(NetworkState::PingForListener, 0);
            }

            break;
        }
    }
}

void NetworkProtocolState::handle(fk_network_packet_t *packet, size_t packetSize) {
    packetsReceived++;

    lastPacketTime = millis();

    DEBUG_PRINT(F("P:"));
    DEBUG_PRINTLN(packet->kind);

    switch (packet->kind) {
    case FK_PACKET_KIND_PING: {
        DEBUG_PRINTLN("Ponging!");

        fk_network_pong_t pong;
        memzero((uint8_t *)&pong, sizeof(fk_network_pong_t));
        pong.fk.kind = FK_PACKET_KIND_PONG;
        pong.time = SystemClock->now();
        radio->reply((uint8_t *)&pong, sizeof(fk_network_pong_t));
        radio->waitPacketSent();
        checkForPacket();
        break;
    }
    case FK_PACKET_KIND_SONAR_STATION: {
        if (state == NetworkState::EnqueueFromNetwork) {
            delay(50);

            diagnostics.recordSonarPacket();

            DEBUG_PRINTLN("Queuing");

            {
                sonar_station_packet_t sonar_station_packet;
                memcpy((void *)&sonar_station_packet, packet, sizeof(sonar_station_packet_t));
                sonar_station_packet.time = SystemClock->now();
                queue->enqueue((uint8_t *)&sonar_station_packet, sizeof(sonar_station_packet_t));
            }

            sendAck();
        }
        break;
    }
    case FK_PACKET_KIND_ATLAS_SENSORS: {
        if (state == NetworkState::EnqueueFromNetwork) {
            delay(50);

            diagnostics.recordAtlasPacket();

            DEBUG_PRINTLN("Queuing");

            queue->enqueue((uint8_t *)packet);

            sendAck();
        }
        break;
    }
    case FK_PACKET_KIND_PONG: {
        if (state == NetworkState::ListenForPong) {
            // M0 requires word aligned access!
            fk_network_pong_t pong;
            memcpy(&pong, packet, sizeof(fk_network_pong_t));

            DEBUG_PRINTLN("Ponged");

            if (pong.time > 0) {
                SystemClock->set(pong.time);
            }

            queue->startAtBeginning();
            dequeueAndSend();
        }
        else {
            DEBUG_PRINTLN(F("Ignore pong"));
        }
        break;
    }
    case FK_PACKET_KIND_ACK: {
        DEBUG_PRINTLN(F("Ack"));
        dequeueAndSend();
        break;
    }
    case FK_PACKET_KIND_FORCE_TRANSMISSION: {
        if (networkCallbacks != NULL) {
            DEBUG_PRINTLN(F("FORCE_TX"));
            networkCallbacks->forceTransmission(this);
            sendAck();
        }
        break;
    }
    default: {
        DEBUG_PRINT(F("Unknown: "));
        DEBUG_PRINTLN(packetSize);

        // So this is actually weird. I'm thinking that sometimes things get corrupted and we
        // should avoid that rather than causing the remote end to retry all over again. I saw
        // this happen during testing (ie restarting during writes, etc..) and I'm just worried
        // we'll see this in production.
        sendNack(FK_PACKET_NACK_KIND);
        break;
    }
    }
}

void NetworkProtocolState::sendPing() {
    DEBUG_PRINTLN(F("Pinging"));

    fk_network_ping_t ping;
    memzero((uint8_t *)&ping, sizeof(fk_network_ping_t));
    ping.fk.kind = FK_PACKET_KIND_PING;
    radio->send((uint8_t *)&ping, sizeof(fk_network_ping_t));
    radio->waitPacketSent();
    checkForPacket();
}

void NetworkProtocolState::sendAck() {
    DEBUG_PRINTLN(F("Acking"));

    fk_network_ack_t ack;
    memzero((uint8_t *)&ack, sizeof(fk_network_ack_t));
    ack.fk.kind = FK_PACKET_KIND_ACK;
    radio->reply((uint8_t *)&ack, sizeof(fk_network_ack_t));
    radio->waitPacketSent();
    checkForPacket();
}

void NetworkProtocolState::sendNack(uint8_t status) {
    DEBUG_PRINTLN(F("Acking"));

    fk_network_ack_t ack;
    memzero((uint8_t *)&ack, sizeof(fk_network_ack_t));
    ack.fk.kind = FK_PACKET_KIND_NACK;
    // ack.status = status;
    radio->reply((uint8_t *)&ack, sizeof(fk_network_ack_t));
    radio->waitPacketSent();
    checkForPacket();
}

void sendNack(uint8_t status);
void NetworkProtocolState::checkForPacket() {
    radio->tick();

    if (radio->hasPacket()) {
        handle((fk_network_packet_t *)radio->getPacket(), radio->getPacketSize());
        radio->clear();
    }
}

void NetworkProtocolState::startOver(NetworkState newState) {
    packetsReceived = 0;
    transition(newState, 0);
}

void NetworkProtocolState::transition(NetworkState newState, uint32_t newDelay) {
    state = newState;
    stateDelay = newDelay;
    lastTickNonDelayed = millis();
}

void NetworkProtocolState::dequeueAndSend() {
    if (state == NetworkState::ListenForPong || state == NetworkState::ListenForAck) {
        while (true) {
            fk_network_packet_t *packet = (fk_network_packet_t *)queue->dequeue();
            if (packet != NULL) {
                if (packet->kind != FK_PACKET_KIND_ACK &&
                    packet->kind != FK_PACKET_KIND_PING &&
                    packet->kind != FK_PACKET_KIND_PONG) {

                    size_t packetSize = fk_packet_get_size(packet);
                    radio->reply((uint8_t *)packet, packetSize);
                    radio->waitPacketSent();
                    checkForPacket();
                    packet = NULL;

                    transition(NetworkState::ListenForAck, 5000);

                    break;
                }
                else {
                    DEBUG_PRINTLN(F("Ign non-ACK"));
                }
            }
            else {
                DEBUG_PRINTLN(F("Empty"));
                radio->sleep();
                transition(NetworkState::QueueEmpty, 0);
                break;
            }
        }
    }
}
