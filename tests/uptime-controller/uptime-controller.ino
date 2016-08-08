#include "RTClib.h"

#define CMD_READING     "reading "
#define CMD_OFF         "off "
#define CMD_ON          "on "
#define CMD_HIBERNATE   "hibernate "
#define CMD_TIME        "time "
#define CMD_NOW         "now"
#define CMD_HUP         "hup"

#define PIN_PI_STATUS   11
#define PIN_PI_POWER    10

RTC_DS1307 rtc;

String command = "";

void printTime(const DateTime &time) {
    Serial.print(time.year(), DEC);
    Serial.print('/');
    Serial.print(time.month(), DEC);
    Serial.print('/');
    Serial.print(time.day(), DEC);
    Serial.print(" ");
    Serial.print(time.hour(), DEC);
    Serial.print(':');
    Serial.print(time.minute(), DEC);
    Serial.print(':');
    Serial.print(time.second(), DEC);
}

void setup() {
    pinMode(PIN_PI_STATUS, INPUT);

    Serial.begin(115200);

    if (!rtc.begin()) {
        Serial.println("RTC Missing");
    }
    else {
        Serial.println("RTC ready");
        printTime(rtc.now());
        Serial.println();
    }

    if (digitalRead(PIN_PI_STATUS)) {
        Serial.println("Pi is on, leaving Pi alone.");
        turnPiOn();
    }
    else {
        Serial.println("Pi is off, keeping Pi off.");
        pinMode(PIN_PI_POWER, OUTPUT);
        digitalWrite(PIN_PI_POWER, LOW);
    }

    Serial.println("READY");
}

void lowPowerDelay(uint32_t ms) {
    uint32_t started = millis();
    while (millis() - started < ms) {
        delay(100);
    }
}

void turnPiOff() {
    pinMode(PIN_PI_POWER, OUTPUT);
    digitalWrite(PIN_PI_POWER, LOW);
}

void turnPiOn() {
    pinMode(PIN_PI_POWER, INPUT);
}

void handleCommand(String &line) {
    Serial.println(line);

    if (line.startsWith(CMD_ON)) {
        uint32_t msDelay = line.substring(strlen(CMD_ON)).toInt();
        Serial.print("powering on after ");
        Serial.println(msDelay);
        Serial.println("OK");

        lowPowerDelay(msDelay);

        turnPiOn();
    }
    else if (line.startsWith(CMD_OFF)) {
        uint32_t msDelay = line.substring(strlen(CMD_OFF)).toInt();
        Serial.print("powering off after ");
        Serial.println(msDelay);
        Serial.println("OK");

        lowPowerDelay(msDelay);

        turnPiOff();
    }
    else if (line.startsWith(CMD_HIBERNATE)) {
        String times = line.substring(strlen(CMD_HIBERNATE));
        int32_t spaceIndex = times.indexOf(' ');
        if (spaceIndex > 0) {
            uint32_t offDelay = times.substring(0, spaceIndex).toInt();
            uint32_t onDelay = times.substring(spaceIndex).toInt();

            Serial.print("hibernating, off in ");
            Serial.print(offDelay);
            Serial.print(" and on in ");
            Serial.println(onDelay);
            Serial.println("OK");

            lowPowerDelay(offDelay);

            turnPiOff();

            lowPowerDelay(onDelay);

            turnPiOn();
        }
    }
    else if (line.startsWith(CMD_NOW)) {
        printTime(rtc.now());
        Serial.println();
        Serial.println("OK");
    }
    else if (line.startsWith(CMD_TIME)) {
        String newTime = line.substring(strlen(CMD_TIME));
        if (newTime.length() != 0) {
            // date = "Dec 26 2009", time = "12:34:56"
            rtc.adjust(DateTime(newTime.substring(0, 11).c_str(), newTime.substring(12).c_str()));

            printTime(rtc.now());
            Serial.println();
            Serial.println("OK");
        }
        else {
            Serial.println("ERROR");
        }
    }
    else if (line.startsWith(CMD_HUP)) {
        delay(100);

        (*(void(*)())0)();
    }
    else {
        if (line.length() > 0) {
            Serial.println("ERROR");
        }
    }
}

void loop() {
    delay(10);

    if (Serial.available()) {
        int32_t c = Serial.read();
        if (c > 0) {
            Serial.print((char)c);
            if (c == '\r') {
                handleCommand(command);
                command = "";
            }
            else {
                command += (char)c;
            }
        }
    }
}

// vim: set ft=cpp:
