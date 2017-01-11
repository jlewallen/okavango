#include <Adafruit_SleepyDog.h>

#include "Platforms.h"

#define BATTERY_VOLTAGE_DIVIDER_RATIO                         2.0f
#define BATTERY_VOLTAGE_REFERENCE                             3.3f
#define BATTERY_VOLTAGE_OPTIMAL                               4.2f
#define BATTERY_VOLTAGE_LOW                                   3.0f


#ifdef ARDUINO_SAMD_FEATHER_M0

// PA16 = 11  SERCOM1/ PAD[0] SERCOM3/ PAD[0]
// PA18 = 10  SERCOM1/ PAD[2] SERCOM3/ PAD[2]
Uart Serial2(&sercom1, 11, 10, SERCOM_RX_PAD_0, UART_TX_PAD_2);

void SERCOM1_Handler()
{
    Serial2.IrqHandler();
}

/*
// PA23 = 21   SERCOM3/ PAD[1] SERCOM5/ PAD[1]
// PA22 = 20   SERCOM3/ PAD[0] SERCOM5/ PAD[0]
Uart Serial3(&sercom3, 21, 20, SERCOM_RX_PAD_1, UART_TX_PAD_0);

void SERCOM3_Handler()
{
    Serial3.IrqHandler();
}
*/

SerialType &portExpanderSerial = Serial1;
SerialType &conductivitySerial = Serial2;

void platformSerial2Begin(int32_t baud) {
    Serial2.begin(baud);

    // Order is very important here. This has to happen after the call to begin.
    pinPeripheral(10, PIO_SERCOM);
    pinPeripheral(11, PIO_SERCOM);
}

/*
void platformSerial3Begin(int32_t baud) {
    Serial3.begin(baud);

    // Order is very important here. This has to happen after the call to begin.
    pinPeripheral(21, PIO_SERCOM);
    pinPeripheral(20, PIO_SERCOM);
}
*/

extern "C" char *sbrk(int32_t i);

uint32_t platformFreeMemory() {
    char stack_dummy = 0;
    return &stack_dummy - sbrk(0);
}
void platformRestart() {
    NVIC_SystemReset();
}

float platformBatteryVoltage() {
    analogRead(A1);
    delay(2);
    analogRead(A1);
    delay(2);
    float value = analogRead(A1);
    return value * BATTERY_VOLTAGE_DIVIDER_RATIO * BATTERY_VOLTAGE_REFERENCE / 1024.0f;
}

float platformBatteryLevel() {
    float constrained = max(min(platformBatteryVoltage(), BATTERY_VOLTAGE_OPTIMAL), BATTERY_VOLTAGE_LOW);
    return (constrained - BATTERY_VOLTAGE_LOW) / (BATTERY_VOLTAGE_OPTIMAL - BATTERY_VOLTAGE_LOW);
}

#endif

void platformBlinks(uint8_t pin, uint8_t number) {
    for (uint8_t i = 0; i < number; ++i) {
        delay(250);
        digitalWrite(pin, HIGH);
        delay(250);
        digitalWrite(pin, LOW);
    }
}

void platformBlink(uint8_t pin) {
    delay(500);
    digitalWrite(pin, HIGH);
    delay(500);
    digitalWrite(pin, LOW);
}

void platformPulse(uint8_t pin) {
    delay(1000);
    analogWrite(pin, 256);
    delay(1000);
    analogWrite(pin, 128);
}

void platformCatastrophe(uint8_t pin, uint8_t mode) {
    uint32_t watchdogMs = Watchdog.enable();
    uint32_t restartAfter = (30 * 1000) - watchdogMs;
    DEBUG_PRINTLN("Catastrophe!");
    uint32_t started = millis();
    while (true) {
        if (millis() - started < restartAfter) {
            Watchdog.reset();
        }
        else {
            pinMode(PIN_POWER_HARD_RESET, OUTPUT);
            digitalWrite(PIN_POWER_HARD_RESET, HIGH);
        }
        switch (mode) {
        case PLATFORM_CATASTROPHE_FAST_BLINK:
            delay(100);
            digitalWrite(pin, HIGH);
            delay(100);
            digitalWrite(pin, LOW);
            break;
        default:
            platformPulse(pin);
            break;
        }
    }
}

uint32_t platformWaitForBattery() {
    if (platformBatteryLevel() > 0.1) {
        return 0;
    }

    uint32_t started = millis();
    while (platformBatteryLevel() > 0.1) {
        delay(5000);
    }
    return millis() - started;
}

void platformLowPowerSleep(uint32_t numberOfMs) {
    if (numberOfMs > 0) {
        uint32_t slept = 0;
        while (slept < numberOfMs) {
            uint32_t before = millis();
            Watchdog.sleep();
            platformBlink(PIN_SLEEP_LED);
            slept += millis() - before;
        }
    }
}
