#ifndef NETWORK_H
#define NETWORK_H

#include "LoraRadio.h"
#include "Queue.h"
#include "Platforms.h"
#include "protocol.h"

enum NetworkState {
    EnqueueFromNetwork = 0,
    PingForListener = 1,
    ListenForPong = 2,
    ListenForAck = 3,
    GiveListenerABreak = 4,
    Sleep = 5,
    QueueEmpty = 6,
    NobodyListening = 7,
    Quiet = 8
};

typedef struct rf95_header_t {
    uint8_t to;
    uint8_t from;
    uint8_t flags;
    uint8_t id;
    uint8_t rssi;
} rf95_header_t;

class NetworkProtocolState;

class NetworkCallbacks {
public:
    virtual bool forceTransmission(NetworkProtocolState *networknetworkProtocol) = 0;
    virtual void handlePacket(rf95_header_t *header, fk_network_packet_t *packet, size_t packetSize) = 0;
};

class NetworkProtocolState {
private:
    LoraRadio *radio;
    Queue *queue;
    NetworkState state;
    uint16_t stateDelay;
    uint16_t packetsReceived;
    uint32_t lastPacketTime;
    uint32_t lastTick;
    uint32_t lastTickNonDelayed;
    uint32_t startedAt;
    bool pingAgainAfterDequeue;
    NetworkCallbacks *networkCallbacks;
    uint8_t identity;
    bool sendSystemTimeInPong;

public:
    NetworkProtocolState(uint8_t identity, NetworkState state, LoraRadio *radio, Queue *queue, NetworkCallbacks *networkCallbacks, bool sendSystemTimeInPong = true);

public:
    void tick();
    void startOver(NetworkState state);
    bool beenRunningTooLong();
    bool isQuiet() {
        return state == Quiet;
    }
    bool isQueueEmpty() {
        return state == QueueEmpty;
    }
    bool isNobodyListening() {
        return state == NobodyListening;
    }
    uint16_t numberOfPacketsReceived() {
        return packetsReceived;
    }

private:
    void sendPing();
    void sendAck();
    void sendNack(uint8_t status);
    void dequeueAndSend();
    void checkForPacket();
    void handle(fk_network_packet_t *packet, size_t packetSize);
    void transition(NetworkState newState, uint32_t delay);
    bool is(NetworkState aState) {
        return state == aState;
    }

};

#endif
