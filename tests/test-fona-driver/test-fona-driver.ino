#define WAIT_FOR_SERIAL   1000 * 20

#include "Platforms.h"
#include "NonBlockingSerial.h"

#define NUMBER_TO_SMS       ""

enum FonaChildState {
    Start,
    Power,
    NetworkStatus,
    WaitForNetwork,
    SendSms,
    Done
};

class FonaChild : public NonBlockingSerialProtocol {
private:
    FonaChildState state = Start;
    uint32_t lastStateChange;
    bool registered;

public:
    virtual bool tick();
    virtual bool handle(String reply);

    void transition(FonaChildState newState) {
        state = newState;
        lastStateChange = millis();
    }

    bool isDone() {
        return state == Done;
    }

};

bool FonaChild::tick() {
    if (NonBlockingSerialProtocol::tick()) {
        return true;
    }
    switch (state) {
        case Start: {
            sendCommand("~HELLO");
            registered = false;
            break;
        }
        case Power: {
            sendCommand("~POWER");
            break;
        }
        case NetworkStatus: {
            sendCommand("~STATUS");
            break;
        }
        case WaitForNetwork: {
            if (millis() - lastStateChange > 2000) {
                transition(NetworkStatus);
            }
            break;
        }
        case SendSms: {
            String command = "~SMS " + NUMBER_TO_SMS + " WORD";
            sendCommand(command);
            break;
        }
    }
    return true;
}

bool FonaChild::handle(String reply) {
    Serial.print(">");
    Serial.println(reply);
    if (reply.startsWith("OK")) {
        switch (state) {
            case Start: {
                transition(Power);
                break;
            }
            case Power: {
                transition(NetworkStatus);
                break;
            }
            case NetworkStatus: {
                if (registered) {
                    transition(SendSms);
                }
                else {
                    transition(WaitForNetwork);
                }
                break;
            }
            case SendSms: {
                transition(Done);
                break;
            }
            case Done: {
                break;
            }
        }
        return true;
    }
    else if (reply.startsWith("+STATUS")) {
        uint8_t comma = reply.indexOf(",");
        uint8_t status = reply.substring(comma + 1, comma + 2).toInt(); 
        registered = status == 1 || status == 5; // Home or Roaming
    }
    return false;
}

FonaChild fona;

void setup() {
    Serial.begin(115200);

    #ifdef WAIT_FOR_SERIAL
    while (!Serial) {
        delay(100);
        if (millis() > WAIT_FOR_SERIAL) {
            break;
        }
    }
    #endif

    Serial1.begin(4800);

    SerialType &fonaSerial = Serial1;
    fona.setSerial(&fonaSerial);
}

void loop() {
    fona.tick();
}

// vim: set ft=cpp:
