#ifndef COLLECTOR_NETWORK_CALLBACKS_H_INCLUDED
#define COLLECTOR_NETWORK_CALLBACKS_H_INCLUDED

class CollectorNetworkCallbacks : public NetworkCallbacks {
    bool transmissionForced = false;

    virtual bool forceTransmission(NetworkProtocolState *networkProtocol) {
        transmissionForced = true;
        return true;
    }
};

#endif
