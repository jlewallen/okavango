#include "fona.h"

FonaChild::FonaChild(String phoneNumber, String message) :
    phoneNumber(phoneNumber), message(message), NonBlockingSerialProtocol(true, false), tries(0) {
}

bool FonaChild::tick() {
    if (NonBlockingSerialProtocol::tick()) {
        return true;
    }

    switch (state) {
        case Start: {
            registered = false;
            if (tries++ > 3) {
                transition(PowerOffBeforeFailed);
            }
            else {
                sendCommand("~HELLO");
            }
            break;
        }
        case Power: {
            sendCommand("~POWER");
            break;
        }
        case NetworkStatus: {
            if (tries++ > 10) {
                transition(Failed);
            }
            else {
                sendCommand("~STATUS");
            }
            break;
        }
        case WaitForNetwork: {
            if (millis() - lastStateChange > 2000) {
                transition(NetworkStatus);
            }
            break;
        }
        case SendSms: {
            if (message.length() > 0) {
                String command = "~SMS " + phoneNumber + " " + message;
                sendCommand(command.c_str());
            }
            else {
                sendCommand("~STATUS");
            }
            break;
        }
        case PowerOffBeforeFailed: {
            sendCommand("~OFF");
            break;
        }
        case PowerOffBeforeDone: {
            sendCommand("~OFF");
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
                tries = 0;
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
            case PowerOffBeforeFailed: {
                transition(Failed);
                break;
            }
            case PowerOffBeforeDone: {
                transition(Done);
                break;
            }
            case Done: {
                break;
            }
        }
        return true;
    }
    else if (reply.startsWith("ER")) {
        switch (state) {
            case SendSms: {
                transition(PowerOffBeforeFailed);
                break;
            }
            case PowerOffBeforeDone:
            case PowerOffBeforeFailed: {
                transition(Failed);
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

