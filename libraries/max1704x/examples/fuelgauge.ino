#include <Arduino.h>
#include <SD.h>
#include <RTClib.h>

#include "FuelGauge.h"

RTC_PCF8523 rtc;

void setup() {
    pinMode(13, OUTPUT);

    Serial.begin(115200);

    while (!Serial && millis() < 10 * 1000) {
        delay(100);
    }

    if (!rtc.begin()) {
        Serial.println("RTC Missing");
    }
    else {
        Serial.println("RTC Good");
    }

    pinMode(8, OUTPUT);
    digitalWrite(8, HIGH);

    pinMode(4, OUTPUT);
    digitalWrite(4, HIGH);
    if (!SD.begin(4)) {
        pinMode(16, OUTPUT);
        digitalWrite(16, HIGH);
        if (!SD.begin(16)) {
            while (true) {
                delay(500);
                digitalWrite(13, LOW);
                delay(500);
                digitalWrite(13, HIGH);
            }
        }
    }

    Serial.println("gauge: PoR");

    File log = SD.open("BATT.LOG", FILE_WRITE);
    if (!log) {
        while (true) {
            delay(500);
            digitalWrite(13, LOW);
            delay(500);
            digitalWrite(13, HIGH);
        }
    }

    Wire.begin();

    FuelGauge gauge;
    gauge.powerOn();

    Serial.println("gauge: Version");
    gauge.version();

    Serial.println("gauge: Config");
    gauge.config();

    Serial.println("gauge: Reading");

    while (true) {
        float soc = gauge.stateOfCharge();
        float cellVoltage = gauge.cellVoltage();
        Serial.print("gauge: ");
        Serial.print(soc);
        Serial.print(" ");
        Serial.print(cellVoltage);
        Serial.println();

        log.print("gauge: ");
        log.print(soc);
        log.print(" ");
        log.print(cellVoltage);
        log.println();
        log.flush();

        digitalWrite(13, HIGH);
        delay(500);
        digitalWrite(13, LOW);

        delay(5000);
    }
}

void loop() {

}
