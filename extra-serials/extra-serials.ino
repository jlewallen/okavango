#include "wiring_private.h" // pinPeripheral() function

#define PIN 13

Uart Serial2(&sercom1, 11, 10, SERCOM_RX_PAD_0, UART_TX_PAD_2);
void SERCOM1_Handler()
{
    Serial2.IrqHandler();
}

void setup() {
    pinMode(PIN, OUTPUT);
    digitalWrite(PIN, LOW);

    Serial.begin(112500);
    Serial1.begin(9600);
    Serial2.begin(9600);

    // Order is very important here. This has to happen after the call to begin.
    pinPeripheral(10, PIO_SERCOM);
    pinPeripheral(11, PIO_SERCOM);
}

uint32_t flash = 0;
uint32_t stamp = 0;
uint32_t message = 0;
String buffer = "";

void loop() {
    if (Serial2.available() > 0) {
        int16_t c = Serial2.read();
        Serial.print((char)c);
        if (c > 0) {
            buffer += (char)c;
            if (c == '\r') {
                Serial.println(buffer);
                buffer = "";
            }
        }
    }

    if (millis() - stamp > 5000) {
        stamp = millis();
        Serial2.print("STATUS\r");
        Serial.println("STATUS");
    }

    if (millis() - message > 1000) {
        flash++;
        message = millis();
        digitalWrite(PIN, flash % 2 ? HIGH : LOW);
    }
}
