#include <SPI.h>
#include <SD.h>
#include <RH_RF95.h>
#include <Wire.h>
#include "RTClib.h"

#define WAIT_FOR_SERIAL                  30 * 1000
#define RFM95_CS                         8
#define RFM95_RST                        4
#define RFM95_INT                        3
#define PIN_SD_CS                        10

RH_RF95 rf95(RFM95_CS, RFM95_INT);
RTC_PCF8523 rtc;

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

    pinMode(PIN_SD_CS, OUTPUT);
    pinMode(RFM95_CS, OUTPUT);
    pinMode(RFM95_RST, OUTPUT);

    digitalWrite(PIN_SD_CS, HIGH);
    digitalWrite(RFM95_CS, HIGH);
    digitalWrite(RFM95_RST, HIGH);

    Serial.println("Begin");

    if (!rtc.begin()) {
        Serial.println("RTC Missing");
    }
    else {
        if (!rtc.initialized()) {
            Serial.println("RTC uninitialized");
            rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
            // January 21, 2014 at 3am
            // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
        }
        else {
            DateTime now = rtc.now();
            Serial.println("RTC OK");
        }
    }

    if (!SD.begin(PIN_SD_CS)) {
        Serial.println("SD Missing");
    }
    else {
        Serial.println("SD OK");
    }

    digitalWrite(RFM95_RST, LOW);
    delay(10);
    digitalWrite(RFM95_RST, HIGH);

    if (!rf95.init()) {
        Serial.println("LoraRadio: Initialize failed!");
    }
    else {
        rf95.setFrequency(915.0f);
        rf95.setTxPower(23, false);

        Serial.println("Radio OK");
    }
}

void loop() {
    Serial.println("Trying radio...");
    digitalWrite(PIN_SD_CS, HIGH);
    digitalWrite(RFM95_CS, HIGH);

    const char *message = "Hello";
    if (!rf95.send((uint8_t *)message, strlen(message))) {
        Serial.println("Failed");
    }

    Serial.println("Trying SD...");
    digitalWrite(PIN_SD_CS, HIGH);
    digitalWrite(RFM95_CS, HIGH);

    File file = SD.open("test.bin", FILE_WRITE);
    if (file) {
        file.println("Hello, world!");
        file.flush();
        file.close();
    }
    else {
        Serial.println("Failed");
    }

    SD.remove("test.bin");

    Serial.println("Trying RTC...");
    digitalWrite(PIN_SD_CS, HIGH);
    digitalWrite(RFM95_CS, HIGH);

    DateTime now = rtc.now();
    Serial.println(now.unixtime());

    Serial.println("Done");

    delay(1000);
}

// vim: set ft=cpp:
