#include <Arduino.h>
#include <system.h>
#include <wdt.h>
#include "battery.h"

#define HAS_SSD1306

#ifdef HAS_SSD1306
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#endif

#define PIN_LED                                               13
#define PIN_RESET_HARD                                        5
#define PIN_BATTERY_LEVEL                                     A0
#define PIN_OLED_RESET                                        6

#define BATTERY_LEVEL_LOW                                     30
#define BATTERY_LEVEL_VERY_LOW                                10
// #define DISABLE_SLEEP

#ifdef HAS_SSD1306
Adafruit_SSD1306 display(PIN_OLED_RESET);
#endif

#ifdef HAS_SSD1306
void message_show(uint8_t row, const char *message) {
    const uint8_t textSize = 1;
    display.setTextSize(textSize);
    display.setTextColor(WHITE);
    display.setCursor(0, row * 8 * textSize);
    for (uint8_t i = 0; i < strlen(message); i++) {
        display.write(message[i]);
    }
}
#endif

uint8_t sample_battery_level() {
    float total = 0.0f;
    for (uint8_t i = 0; i < 3; ++i) {
        float voltage = battery_voltage_get(PIN_BATTERY_LEVEL);
        total += battery_voltage_to_level(voltage);
    }

    return (uint8_t)((total * 100.0f) / 3.0f);
}

void blink_led(uint8_t times) {
    for (uint8_t i = 0; i < times; ++i) {
        digitalWrite(PIN_LED, HIGH);
        delay(250);
        digitalWrite(PIN_LED, LOW);
        delay(250);
    }
}

void pause_for_charge() {
    while (true) {
        uint8_t level = sample_battery_level();
        if (level > BATTERY_LEVEL_VERY_LOW) {
            break;
        }

        #ifndef DISABLE_SLEEP
        system_deep_sleep();
        #endif

        wdt_reset();

        blink_led(2);

        delay(1000);
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    pinMode(PIN_RESET_HARD, OUTPUT);
    digitalWrite(PIN_RESET_HARD, LOW);

    wdt_enable(WDT_PERIOD_8X);

    #ifdef HAS_SSD1306
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    #endif

    pause_for_charge();

    #ifdef HAS_SSD1306
    display.setRotation(2);
    display.clearDisplay();
    display.display();
    #endif

    uint32_t displayUpdatedAt = 0;
    uint32_t changedAt = 0;

    bool hasVeryLowBattery = true;
    bool hasLowBattery = true;
    bool receivedEarlyWarning = false;
    bool displayOn = false;

    while (true) {
        if (millis() - displayUpdatedAt > 1000) {
            float voltage = battery_voltage_get(PIN_BATTERY_LEVEL);
            uint8_t level = (uint8_t)(battery_voltage_to_level(voltage) * 100);
            hasLowBattery = level < BATTERY_LEVEL_LOW;
            hasVeryLowBattery = level < BATTERY_LEVEL_VERY_LOW;

            String voltageDisplay(voltage, 2);
            String levelDisplay(level);
            String batteryDisplay =  levelDisplay + "% " + voltageDisplay;
            String uptimeDisplay(millis() / 1000);
            String statusDisplay = String(hasLowBattery ? "low battery" : "ok") + String(receivedEarlyWarning ? " wd" : "");

            if (!hasVeryLowBattery && !displayOn) {
                Serial.println("Display on");
                display.ssd1306_command(SSD1306_DISPLAYON);
                displayOn = true;
            }

            #ifdef HAS_SSD1306
            display.clearDisplay();
            message_show(0, batteryDisplay.c_str());
            message_show(1, uptimeDisplay.c_str());
            message_show(2, statusDisplay.c_str());
            display.dim(hasLowBattery);
            display.display();
            #else
            Serial.print("Battery: voltage=");
            Serial.print(voltageDisplay);
            Serial.print(" level=");
            Serial.print(levelDisplay);
            Serial.println();
            #endif

            if (hasVeryLowBattery && displayOn) {
                Serial.println("Display off");
                display.ssd1306_command(SSD1306_DISPLAYOFF);
                displayOn = false;
            }

            displayUpdatedAt = millis();
            receivedEarlyWarning = false;

            pause_for_charge();
        }

        if (wdt_read_early_warning()) {
            receivedEarlyWarning = true;
            wdt_reset();
        }

        delay(10);
    }
}

void loop() {

}
