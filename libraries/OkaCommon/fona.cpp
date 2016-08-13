#include "fona.h"

FonaChild::FonaChild(String phoneNumber, String message) :
    phoneNumber(phoneNumber), message(message),
    NonBlockingSerialProtocol(60 * 1000, true, false), tries(0) {
}

void FonaChild::drain() {
    uint32_t started = millis();

    while (millis() - started < 500) {
        delay(10);
        while (getSerial()->available()) {
            getSerial()->read();
        }
    }
}

bool FonaChild::tick() {
    if (NonBlockingSerialProtocol::tick()) {
        return true;
    }

    if (getSendsCounter() == 5) {
        transition(FonaDone);
        return true;
    }

    switch (state) {
        case FonaStart: {
            registered = false;
            if (tries++ > 3) {
                transition(FonaPowerOffBeforeFailed);
            }
            else {
                drain();
                sendCommand("~HELLO");
            }
            break;
        }
        case FonaPower: {
            sendCommand("~POWER");
            break;
        }
        case FonaNetworkStatus: {
            if (tries++ > 10) {
                transition(FonaPowerOffBeforeFailed);
            }
            else {
                sendCommand("~STATUS");
            }
            break;
        }
        case FonaWaitForNetwork: {
            if (millis() - lastStateChange > 2000) {
                transition(FonaNetworkStatus);
            }
            break;
        }
        case FonaSendSms: {
            if (message.length() > 0) {
                String command = "~SMS " + phoneNumber + " " + message;
                sendCommand(command.c_str());
            }
            else {
                sendCommand("~STATUS");
            }
            break;
        }
        case FonaPowerOffBeforeFailed: {
            sendCommand("~OFF");
            break;
        }
        case FonaPowerOffBeforeDone: {
            sendCommand("~OFF");
            break;
        }
    }
    return true;
}

bool FonaChild::handle(String reply) {
    if (reply.length() > 0) {
        Serial.print(state);
        Serial.print(">");
        Serial.println(reply);
    }
    if (reply.startsWith("OK")) {
        switch (state) {
            case FonaStart: {
                transition(FonaPower);
                break;
            }
            case FonaPower: {
                transition(FonaNetworkStatus);
                tries = 0;
                break;
            }
            case FonaNetworkStatus: {
                if (registered) {
                    transition(FonaSendSms);
                }
                else {
                    transition(FonaWaitForNetwork);
                }
                break;
            }
            case FonaSendSms: {
                transition(FonaPowerOffBeforeDone);
                break;
            }
            case FonaPowerOffBeforeFailed: {
                transition(FonaFailed);
                break;
            }
            case FonaPowerOffBeforeDone: {
                transition(FonaDone);
                break;
            }
            case FonaDone: {
                break;
            }
        }
        return true;
    }
    else if (reply.startsWith("ER")) {
        switch (state) {
            case FonaSendSms: {
                transition(FonaPowerOffBeforeFailed);
                break;
            }
            case FonaPowerOffBeforeDone:
            case FonaPowerOffBeforeFailed: {
                transition(FonaFailed);
                break;
            }
        }
        return true;
    }
    else if (reply.startsWith("+STATUS")) {
        int8_t comma = reply.indexOf(",");
        int8_t status = reply.substring(comma + 1, comma + 2).toInt(); 
        registered = status == 1 || status == 5; // Home or Roaming
    }
    return false;
}

