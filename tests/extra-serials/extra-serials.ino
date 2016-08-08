#include "wiring_private.h" // pinPeripheral() function

#define PIN 13

/*
typedef enum
{
	SERCOM_RX_PAD_0 = 0,
	SERCOM_RX_PAD_1,
	SERCOM_RX_PAD_2,
	SERCOM_RX_PAD_3
} SercomRXPad;
 
typedef enum
{
	UART_TX_PAD_0 = 0x0ul,	// Only for UART
	UART_TX_PAD_2 = 0x1ul,  // Only for UART
	UART_TX_RTS_CTS_PAD_0_2_3 = 0x2ul,  // Only for UART with TX on PAD0, RTS on PAD2 and CTS on PAD3
} SercomUartTXPad;
*/

#define PIN_SERIAL2_RX 11
#define PIN_SERIAL2_TX 10

// Rx = 11
// Tx = 10
Uart Serial2(&sercom1, PIN_SERIAL2_RX, PIN_SERIAL2_TX, SERCOM_RX_PAD_0, UART_TX_PAD_2);
void SERCOM1_Handler()
{
    Serial2.IrqHandler();
}

#define PIN_SERIAL3_RX 24
#define PIN_SERIAL3_TX 22
// Rx = 24
// Tx = 15
Uart Serial3(&sercom4, PIN_SERIAL3_RX, PIN_SERIAL3_TX, SERCOM_RX_PAD_3, UART_TX_PAD_0);
void SERCOM3_Handler()
{
    Serial3.IrqHandler();
}

void setup() {
    pinMode(PIN, OUTPUT);
    digitalWrite(PIN, LOW);

    while (!Serial)
    {
    }

    Serial.println("Go");

    Serial.begin(112500);
    Serial1.begin(9600);
    Serial2.begin(9600);
    Serial3.begin(9600);

    Serial.println("Ready");

    // Order is very important here. This has to happen after the call to begin.
    pinPeripheral(PIN_SERIAL2_RX, PIO_SERCOM);
    pinPeripheral(PIN_SERIAL2_TX, PIO_SERCOM);

    pinPeripheral(PIN_SERIAL3_RX, PIO_SERCOM);
    pinPeripheral(PIN_SERIAL3_TX, PIO_SERCOM);

    Serial.println("Running");
}

uint32_t flash = 0;
uint32_t stamp = 0;
uint32_t message = 0;
String buffer = "";

void loop() {
    /*
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
    */

    if (Serial3.available() > 0) {
        int16_t c = Serial3.read();
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
        // Serial2.print("STATUS\r");
        Serial.println("STATUS");
        Serial3.print("STATUS\r");
        Serial.println("Ok");
    }

    if (millis() - message > 1000) {
        flash++;
        message = millis();
        digitalWrite(PIN, flash % 2 ? HIGH : LOW);
    }
}

// vim: set ft=cpp:
