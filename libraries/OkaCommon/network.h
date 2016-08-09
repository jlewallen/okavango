#ifndef NETWORK_H
#define NETWORK_H

#include "core.h"
#include "protocol.h"

enum NetworkState {
    EnqueueFromNetwork,
    PingForListener,
    ListenForPong,
    ListenForAck,
    GiveListenerABreak,
    Sleep,

    QueueEmpty,
    NobodyListening
};

class NetworkProtocolState {
private:
    CorePlatform *platform;
    NetworkState state;
    uint16_t stateDelay;
    uint16_t packetsReceived;
    uint32_t lastTick;
    uint32_t lastTickNonDelayed;
    bool pingAgainAfterDequeue;
    
public:
    NetworkProtocolState(NetworkState state, CorePlatform *platform);

public:
    void tick();
    void startOver(NetworkState state);
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
    void dequeueAndSend();
    void checkForPacket();
    void handle(fk_network_packet_t *packet);
    void transition(NetworkState newState, uint32_t delay);
    bool is(NetworkState aState) {
        return state == aState;
    }

};

#endif
