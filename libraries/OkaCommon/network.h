#ifndef NETWORK_H
#define NETWORK_H

#include "LoraRadio.h"
#include "Queue.h"
#include "Platforms.h"
#include "protocol.h"

enum NetworkState {
    EnqueueFromNetwork,
    PingForListener,
    ListenForPong,
    ListenForAck,
    GiveListenerABreak,
    Sleep,
    QueueEmpty,
    NobodyListening,
    Quiet
};

class NetworkProtocolState;

class NetworkCallbacks {
public:
    virtual bool forceTransmission(NetworkProtocolState *networknetworkProtocol) = 0;
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
    bool pingAgainAfterDequeue;
    NetworkCallbacks *networkCallbacks;
    
public:
    NetworkProtocolState(NetworkState state, LoraRadio *radio, Queue *queue, NetworkCallbacks *networkCallbacks);

public:
    void tick();
    void startOver(NetworkState state);
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
