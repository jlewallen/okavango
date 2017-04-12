#include <Arduino.h>
#include <Wire.h>
#include "RTClib.h"

RTC_PCF8523 rtc;

int32_t readInteger(String prompt, int32_t defaultValue) {
    Serial.print(prompt);
    Serial.print("(enter for ");
    Serial.print(defaultValue);
    Serial.print("): ");

    String str = "";
    while (true) {
        while (Serial.available()) {
            char c = Serial.read();
            if (c == '\r') {
                Serial.println();
                if (str.length() == 0) {
                    return defaultValue;
                }
                return str.toInt();
            }
            Serial.print(c);
            str += c;
        }
    }
    return 0;
}

DateTime readDateAndTime(DateTime defaults) {
    int32_t year = readInteger("Year", defaults.year());
    int32_t month = readInteger("Month", defaults.month());
    int32_t day = readInteger("Day", defaults.day());
    int32_t hour = readInteger("Hour", defaults.hour());
    int32_t minute = readInteger("Minute", defaults.minute());
    int32_t second = readInteger("Second", defaults.second());

    DateTime desired(year, month, day, hour, minute, second);

    return desired;
}

void log(DateTime value) {
    Serial.print(value.year(), DEC);
    Serial.print('/');
    Serial.print(value.month(), DEC);
    Serial.print('/');
    Serial.print(value.day(), DEC);
    Serial.print(" ");
    Serial.print(value.hour(), DEC);
    Serial.print(':');
    Serial.print(value.minute(), DEC);
    Serial.print(':');
    Serial.print(value.second(), DEC);
    Serial.println();
}

void setup() {

    Serial.begin(115200);

    while (!Serial) {
        delay(100);
    }

    if (!rtc.begin()) {
        Serial.println("RTC Missing");
        while (true);
    }

    if (!rtc.initialized()) {
        Serial.println("RTC Not running.");
    }
    else {
        Serial.print("Current: ");

        log(rtc.now());
    }

    DateTime defaults(F(__DATE__), F(__TIME__));

    while (true) {
        DateTime newDateAndTime = readDateAndTime(defaults);

        rtc.adjust(newDateAndTime);

        Serial.print("Now: ");

        log(rtc.now());

        Serial.println();

        defaults = newDateAndTime;
    }
}

void loop() {

}
