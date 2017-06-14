#ifndef COLLECTOR_NETWORK_CALLBACKS_H_INCLUDED
#define COLLECTOR_NETWORK_CALLBACKS_H_INCLUDED

class CollectorNetworkCallbacks : public NetworkCallbacks {
    bool transmissionForced = false;

    virtual bool forceTransmission(NetworkProtocolState *networkProtocol) override {
        transmissionForced = true;
        return true;
    }

    virtual void handlePacket(rf95_header_t *header, fk_network_packet_t *packet, size_t packetSize) override {

    }
};

#endif
