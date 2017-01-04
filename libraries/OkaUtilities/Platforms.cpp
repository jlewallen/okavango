#include <Adafruit_SleepyDog.h>

#include "Platforms.h"

uint32_t TransmissionIntervals[] = {
    1000 * 60 * 60 * 24, // * 24 * 7,
    1000 * 60 * 60 * 1
};

#ifdef ARDUINO_AVR_FEATHER32U4

void platformRestart() {
    (*(void(*)())0)();
}

float platformBatteryVoltage() {
    return 0.0f;
}

#endif

#ifdef ARDUINO_AVR_UNO

SerialType portExpanderSerial(PORT_EXPANDER_RX_PIN, PORT_EXPANDER_TX_PIN);
SerialType conductivitySerial(CONDUCTIVITY_RX_PIN, CONDUCTIVITY_TX_PIN);

void platformRestart() {
    (*(void(*)())0)();
}

float platformBatteryVoltage() {
    return 0.0f;
}

uint32_t platformFreeMemory() {
    return 0;
}

#endif

#ifdef ARDUINO_SAMD_FEATHER_M0

Uart Serial2(&sercom1, 11, 10, SERCOM_RX_PAD_0, UART_TX_PAD_2);

void SERCOM1_Handler()
{
    Serial2.IrqHandler();
}

SerialType &portExpanderSerial = Serial1;
SerialType &conductivitySerial = Serial2;

void platformSerial2Begin(int32_t baud) {
    Serial2.begin(baud);

    // Order is very important here. This has to happen after the call to begin.
    pinPeripheral(10, PIO_SERCOM);
    pinPeripheral(11, PIO_SERCOM);
}

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
    return analogRead(A1);
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


#include <SD.h>

bool fileAvailable = false;
File fileLog;
LogPrinter logPrinter;

bool LogPrinter::open() {
    fileLog = SD.open("DEBUG.LOG", FILE_WRITE);
    if (fileLog) {
        fileAvailable = true;
    }
    else {
        fileAvailable = false;
    }
    return fileAvailable;
}

void LogPrinter::flush() {
    if (fileAvailable) {
        fileLog.flush();
    }
    Serial.flush();
}

int LogPrinter::available() {
    return 0;
}

int LogPrinter::read() {
    return -1;
}

int LogPrinter::peek() {
    return -1;
}

size_t LogPrinter::write(uint8_t c) {
    if (fileAvailable) {
        size_t w = fileLog.write(c);
        Serial.write(c);
        return w;
    }
    return Serial.write(c);
}

size_t LogPrinter::write(const uint8_t *buffer, size_t size) {
    if (fileAvailable) {
        size_t w = fileLog.write(buffer, size);
        Serial.write(buffer, size);
        return w;
    }
    return Serial.write(buffer, size);
}
