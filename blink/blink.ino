#include "LowPower.h"
#include <SD.h>

#define PIN 13

void lowPowerSleep(short seconds) {
    while (seconds > 0) {
        period_t period;
        if (seconds >= 8) {
            period = SLEEP_8S;
            seconds -= 8;
        }
        else if (seconds >= 4) {
            period = SLEEP_4S;
            seconds -= 4;
        }
        else if (seconds >= 2) {
            period = SLEEP_2S;
            seconds -= 2;
        }
        else {
            period = SLEEP_1S;
            seconds -= 1;
        }

        /*
        #if defined __AVR_ATmega32U4__
        LowPower.idle(period, ADC_OFF, TIMER4_OFF, TIMER3_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART1_OFF, TWI_OFF, USB_OFF);
        #endif

        #if defined __AVR_ATmega328P__
        LowPower.idle(period, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF);
        #endif
        */

        switch (period) {
            case SLEEP_8S:
                delay(8 * 1000);
                break;
            case SLEEP_4S:
                delay(4 * 1000);
                break;
            case SLEEP_2S:
                delay(2 * 1000);
                break;
            case SLEEP_1S:
                delay(1 * 1000);
                break;
        }
    }
}

void setup() {
    Serial.begin(112500);

    pinMode(PIN, OUTPUT);
    digitalWrite(PIN, LOW);

    Serial.println("Hello");
}

void loop() {
    // lowPowerSleep(10);

    Serial.println("Tick");

    digitalWrite(PIN, HIGH);
    delay(1000);
    digitalWrite(PIN, LOW);
    delay(1000);
}
