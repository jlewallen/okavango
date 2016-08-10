#ifndef FONA_H
#define FONA_H

#include "Platforms.h"
#include "NonBlockingSerial.h"

enum FonaChildState {
    Start,
    Power,
    NetworkStatus,
    WaitForNetwork,
    SendSms,
    Failed,
    Done
};

class FonaChild : public NonBlockingSerialProtocol {
private:
    FonaChildState state = Start;
    uint32_t lastStateChange;
    uint8_t tries;
    bool registered;
    String numberToSms;

public:
    FonaChild(String numberToSms);

    virtual bool tick();
    virtual bool handle(String reply);

    void transition(FonaChildState newState) {
        state = newState;
        lastStateChange = millis();
    }

    bool isDone() {
        return state == Done;
    }

    bool isFailed() {
        return state == Failed;
    }

};

#endif
