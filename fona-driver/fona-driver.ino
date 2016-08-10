#include "Adafruit_FONA.h"
#include <SoftwareSerial.h>
#include "Platforms.h"

#define PIN_FONA_RX         9
#define PIN_FONA_TX         8
#define PIN_FONA_RST        4

SoftwareSerial fonaSerial(PIN_FONA_TX, PIN_FONA_RX);
Adafruit_FONA fona(PIN_FONA_RST);
String buffer = "";
bool available = false;

// #define USB_SERIAL

#ifdef USB_SERIAL
Serial_ &commandSerial = Serial;
#else
HardwareSerial &commandSerial = Serial1;
#endif

void setup() {
    fonaSerial.begin(4800);
    Serial1.begin(4800);
    Serial.begin(115200);

    if (!fona.begin(fonaSerial)) {
        Serial.println(F("No FONA"));
        available = false;
    }
    else {
        available = true;
    }
}

void fona_echo_type() {
    uint8_t type = fona.type();
    commandSerial.print(F("+TYPE,"));
    commandSerial.print(type);
    commandSerial.print(F(","));
    switch (type) {
        case FONA800L:   commandSerial.print(F("FONA 800L")); break;
        case FONA800H:   commandSerial.print(F("FONA 800H")); break;
        case FONA808_V1: commandSerial.print(F("FONA 808 (v1)")); break;
        case FONA808_V2: commandSerial.print(F("FONA 808 (v2)")); break;
        case FONA3G_A:   commandSerial.print(F("FONA 3G (American)")); break;
        case FONA3G_E:   commandSerial.print(F("FONA 3G (European)")); break;
        default:         commandSerial.print(F("?")); break;
    }
    commandSerial.print(F("\r"));
}

void loop() {
    if (commandSerial.available()) {
        char c = (char)commandSerial.read();
        buffer += c;
        if (c == '\r') {
            if (buffer == F("~HELLO\r")) {
                commandSerial.print(F("OK\r"));
            }
            else if (buffer == F("~POWER\r")) {
                if (!fona.begin(fonaSerial)) {
                    commandSerial.print(F("ER\r"));
                }
                else {
                    fona_echo_type();

                    char imei[15] = { 0 };
                    uint8_t length = fona.getIMEI(imei);
                    if (length > 0) {
                        commandSerial.print(F("+IMEI: "));
                        commandSerial.print(imei);
                        commandSerial.print(F("\r"));
                        commandSerial.print(F("OK\r"));
                    }
                    else {
                        commandSerial.print(F("ER\r"));
                    }
                }
            }
            else if (buffer == F("~TYPE\r")) {
                fona_echo_type();
                commandSerial.print(F("OK\r"));
            }
            else if (buffer == F("~STATUS\r")) {
                uint8_t n = fona.getNetworkStatus();
                commandSerial.print(F("+STATUS,"));
                commandSerial.print(n);
                commandSerial.print(F(","));
                if (n == 0) commandSerial.print(F("Not registered"));
                if (n == 1) commandSerial.print(F("Registered (home)"));
                if (n == 2) commandSerial.print(F("Not registered (searching)"));
                if (n == 3) commandSerial.print(F("Denied"));
                if (n == 4) commandSerial.print(F("Unknown"));
                if (n == 5) commandSerial.print(F("Registered roaming"));
                commandSerial.print(F("\r"));
                commandSerial.print(F("OK\r"));
            }
            else if (buffer.startsWith(F("~SMS"))) {
                String end = buffer.substring(5);
                uint32_t i = end.indexOf(' ');
                String number = end.substring(0, i);
                String message = end.substring(i);

                commandSerial.print(F("+NUMBER="));
                commandSerial.print(number);
                commandSerial.print(F("\r"));

                commandSerial.print(F("+MESSAGE="));
                commandSerial.print(message);
                commandSerial.print(F("\r"));

                if (fona.sendSMS((char *)number.c_str(), (char *)message.c_str())) {
                    commandSerial.print(F("OK\r"));
                }
                else {
                    commandSerial.print(F("ER\r"));
                }
            }
            else if (buffer == F("~OFF\r")) {
                commandSerial.print(F("OK\r"));
            }
            else if (buffer == F("~ON\r")) {
                commandSerial.print(F("OK\r"));
            }
            else {
                commandSerial.print(F("+UNK '"));
                commandSerial.print(F("'"));
                commandSerial.print(buffer);
                commandSerial.print(F("'\n"));
                commandSerial.print(F("ER\r"));
            }
            Serial.println();
            buffer = "";
        }
    }
    delay(10);
}

// vim: set ft=cpp:
