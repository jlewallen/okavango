#ifndef NETWORK_H
#define NETWORK_H

#include "core.h"
#include "protocol.h"

enum NetworkState {
    Enqueue,
    TryDequeue,
    Listen,
    Sleep
};

class NetworkProtocolState {
private:
    NetworkState state;
    CorePlatform *platform;
    LoraRadio *radio;
    
public:
    NetworkProtocolState(NetworkState state, CorePlatform *platform);

public:
    void tick();
    void handle(fk_network_packet_t *packet);

private:
    void checkForPacket();

};

#endif
