#include <Wire.h>
#include "RTClib.h"
RTC_DS1307 Clock;

void setup () {
    Serial.begin(9600);
    Wire.begin();
    Clock.begin();
    if (! Clock.isrunning()) {
        Serial.println("RTC is NOT running!");
        // following line sets the RTC to the date & time this sketch was compiled
        Clock.adjust(DateTime(__DATE__, __TIME__));
    }
}
void loop () {
    DateTime now = Clock.now();
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
    delay(1000);
}
