#include "fona.h"

FonaChild::FonaChild(String numberToSms) : numberToSms(numberToSms), NonBlockingSerialProtocol(true) {
}

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
            String command = "~SMS " + numberToSms + " WORD";
            sendCommand(command.c_str());
            break;
        }
    }
    return true;
}

bool FonaChild::handle(String reply) {
    if (reply.length() > 0) {
        Serial.print(">");
        Serial.println(reply);
    }
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

