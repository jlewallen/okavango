#include <Adafruit_SleepyDog.h>

#include "Platforms.h"

uint32_t TransmissionIntervals[] = {
    1000 * 60 * 60 * 24 * 7,
    1000 * 60 * 5
};

#ifdef ARDUINO_AVR_FEATHER32U4

void platformPostSetup() {
}

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

void platformPostSetup() {
}

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

void platformPostSetup() {
    // We have to do this before the pinPeripheral calls. On other platforms
    // it's done later by AtlasScientificBoard.
    conductivitySerial.begin(9600);

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
    return 0.0f;
}

#endif

void platformBlink(uint8_t pin) {
    #ifndef LOW_POWER
    delay(500);
    digitalWrite(pin, HIGH);
    delay(500);
    digitalWrite(pin, LOW);
    #endif
}

void platformCatastrophe(uint8_t pin) {
    while (true) {
        platformBlink(pin);
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

