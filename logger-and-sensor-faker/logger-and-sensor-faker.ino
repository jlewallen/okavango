#include <Arduino.h>
#include <Adafruit_SleepyDog.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <SD.h>
#include "Platforms.h"
#include "system.h"

Adafruit_INA219 ina219;

void setup(void) {
    Watchdog.enable();

    Serial.begin(115200);

    uint32_t currentFrequency;

    if (!SD.begin(PIN_SD_CS)) {
        platformCatastrophe(PIN_RED_LED);
    }

    logPrinter.open();

    switch (system_get_reset_cause()) {
    case SYSTEM_RESET_CAUSE_SOFTWARE: logPrinter.println("Software"); break;
    case SYSTEM_RESET_CAUSE_WDT: logPrinter.println("WDT"); break;
    case SYSTEM_RESET_CAUSE_EXTERNAL_RESET: logPrinter.println("External Reset"); break;
    case SYSTEM_RESET_CAUSE_BOD33: logPrinter.println("BOD13"); break;
    case SYSTEM_RESET_CAUSE_BOD12: logPrinter.println("BOD12"); break;
    case SYSTEM_RESET_CAUSE_POR: logPrinter.println("PoR"); break;
    }

    ina219.begin();
    ina219.setCalibration_32V_1A();
}

void loop(void) {
    Watchdog.reset();

    File file = SD.open("LOAD.CSV", FILE_WRITE);
    if (!file) {
        logPrinter.println("Unable to open file");
        platformCatastrophe(PIN_RED_LED);
        return;
    }

    while (true) {
        float shuntVoltage = ina219.getShuntVoltage_mV();
        float busVoltage = ina219.getBusVoltage_V();
        float current = ina219.getCurrent_mA();
        float loadVoltage = busVoltage + (shuntVoltage / 1000);

        file.print(millis());
        file.print(",");
        file.print(busVoltage);
        file.print(",");
        file.print(shuntVoltage);
        file.print(",");
        file.print(loadVoltage);
        file.print(",");
        file.print(current);
        file.println("");
        file.flush();

        delay(1000);
    }
}
