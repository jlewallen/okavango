#ifndef FONA_H
#define FONA_H

#include "Platforms.h"
#include "NonBlockingSerial.h"

enum FonaChildState {
    FonaStart = 0,
    FonaPower,
    FonaNetworkStatus,
    FonaWaitForNetwork,
    FonaNumber,
    FonaMessage,
    FonaSendSms,
    FonaPowerOffBeforeFailed,
    FonaPowerOffBeforeDone,
    FonaFailed,
    FonaDone
};

class FonaChild : public NonBlockingSerialProtocol {
private:
    FonaChildState state = FonaStart;
    uint32_t lastStateChange;
    uint8_t tries;
    bool registered;
    String phoneNumber;
    String message;

public:
    FonaChild(String phoneNumber, String message);

    virtual bool tick();
    virtual bool handle(String reply);

    void transition(FonaChildState newState) {
        state = newState;
        lastStateChange = millis();
        clearSendsCounter();
    }

    bool isDone() {
        return state == FonaDone;
    }

    bool isFailed() {
        return state == FonaFailed;
    }

};

#endif
