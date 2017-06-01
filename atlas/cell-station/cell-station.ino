#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include "Adafruit_FONA.h"
#include "SerialPortExpander.h"
#include "ParallelizedAtlasScientificSensors.h"

#define WAIT_FOR_SERIAL                             5000

#define PIN_FONA_RX                                 9
#define PIN_FONA_TX                                 8
#define PIN_FONA_RST                                4
#define PIN_FONA_RI                                 7
#define PIN_FONA_KEY                                12
#define PIN_PORT_EXPANDER_SELECT_0                  5
#define PIN_PORT_EXPANDER_SELECT_1                  6
#define PIN_FEATHER_32U4_ADALOGGER_WING_SD_CS       20
#define PIN_SD_CS                                   PIN_FEATHER_32U4_ADALOGGER_WING_SD_CS

SoftwareSerial fonaSerial(PIN_FONA_TX, PIN_FONA_RX);
Adafruit_FONA fona(PIN_FONA_RST);
SingleSerialPortExpander serialPortExpander(PIN_PORT_EXPANDER_SELECT_0, PIN_PORT_EXPANDER_SELECT_1, ConductivityConfig::OnSerial2, &Serial1, 1);
ParallelizedAtlasScientificSensors sensorBoard(&Serial, &serialPortExpander, false);

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

    pinMode(PIN_FONA_KEY, OUTPUT);
    digitalWrite(PIN_FONA_KEY, LOW);

    fonaSerial.begin(4800);
    if (!fona.begin(fonaSerial)) {
        Serial.println(F("No FONA"));
        while (true) {

        }
    }

    uint8_t type = fona.type();
    Serial.println(F("FONA Ready"));

    serialPortExpander.setup();
    serialPortExpander.select(0);

    sensorBoard.start();

    while (true) {
        sensorBoard.tick();

        if (sensorBoard.isDone()) {
            byte newPort = serialPortExpander.getPort() + 1;
            serialPortExpander.select(newPort);
            if (newPort < serialPortExpander.getNumberOfPorts()) {
                sensorBoard.start();
            }
            else {
                break;
            }
        }
    }

    while (true) {
        uint8_t n = fona.getNetworkStatus();
        Serial.print(F("Network status "));
        Serial.print(n);
        Serial.print(F(": "));
        if (n == 0) Serial.println(F("Not registered"));
        if (n == 1) {
            Serial.println(F("Registered (home)"));
            break;
        }

        if (n == 2) Serial.println(F("Not registered (searching)"));
        if (n == 3) Serial.println(F("Denied"));
        if (n == 4) Serial.println(F("Unknown"));
        if (n == 5) Serial.println(F("Registered roaming"));

        delay(1000);
    }

    const char *sendTo = "2132617278";
    const char *message = "Hello";
    if (!fona.sendSMS(sendTo, message)) {
        Serial.println(F("Failed"));
    } else {
        Serial.println(F("Sent!"));
    }


}

void loop() {
    delay(500);
}

// vim: set ft=cpp:
