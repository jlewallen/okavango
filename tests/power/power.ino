#include <Arduino.h>
#include <Adafruit_SleepyDog.h>
#include "wdt.h"
#include "system.h"

void setup() {
    Serial.begin(115200);

    while (!Serial && millis() < 20 * 1000) {
        delay(1);
    }

    pinMode(13, OUTPUT);

    digitalWrite(13, HIGH);

    Serial.println(system_get_device_id());
    Serial.println(system_get_free_memory());

    switch (system_get_reset_cause()) {
    case SYSTEM_RESET_CAUSE_SOFTWARE: Serial.println("Software"); break;
    case SYSTEM_RESET_CAUSE_WDT: Serial.println("WDT"); break;
    case SYSTEM_RESET_CAUSE_EXTERNAL_RESET: Serial.println("External Reset"); break;
    case SYSTEM_RESET_CAUSE_BOD33: Serial.println("BOD13"); break;
    case SYSTEM_RESET_CAUSE_BOD12: Serial.println("BOD12"); break;
    case SYSTEM_RESET_CAUSE_POR: Serial.println("PoR"); break;
    }

    wdt_enable(WDT_PERIOD_8X);

    system_deep_sleep();

    digitalWrite(13, LOW);
    delay(500);
    digitalWrite(13, HIGH);
    delay(500);
    digitalWrite(13, LOW);
    delay(500);
    digitalWrite(13, HIGH);

    while (true) {
        delay(500);
        if (Serial.available()) {
            if (Serial.read() == '.') {
                NVIC_SystemReset();
            }
        }

        if (wdt_read_early_warning()) {
            Serial.println("Early");
        }
        else {
            Serial.print(".");
        }
    }
}

void loop() {
}
