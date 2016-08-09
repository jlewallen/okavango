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
    Sleep
};

class NetworkProtocolState {
private:
    CorePlatform *platform;
    NetworkState state;
    uint32_t stateDelay;
    uint32_t lastTick;
    uint32_t lastTickNonDelayed;
    bool pingAgainAfterDequeue;
    
public:
    NetworkProtocolState(NetworkState state, CorePlatform *platform);

public:
    void tick();
    void handle(fk_network_packet_t *packet);

private:
    void sendPing();
    void sendAck();
    void dequeueAndSend();
    void checkForPacket();
    void transition(NetworkState newState, uint32_t delay);
    bool is(NetworkState aState) {
        return state == aState;
    }

};

#endif
