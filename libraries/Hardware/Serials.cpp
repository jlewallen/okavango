#include "Serials.h"

#ifdef ARDUINO_AVR_FEATHER32U4

SoftwareSerial Serial2(11, 10);

void platformSerial2Begin(int32_t baud) {
    Serial2.begin(baud);
}

#endif

#ifdef ARDUINO_SAMD_FEATHER_M0

#include "wiring_private.h" // pinPeripheral() function

// PA16 = 11  SERCOM1/ PAD[0] SERCOM3/ PAD[0]
// PA18 = 10  SERCOM1/ PAD[2] SERCOM3/ PAD[2]
Uart Serial2(&sercom1, 11, 10, SERCOM_RX_PAD_0, UART_TX_PAD_2);

void SERCOM1_Handler()
{
    Serial2.IrqHandler();
}

void platformSerial2Begin(int32_t baud) {
    Serial2.begin(baud);

    // Order is very important here. This has to happen after the call to begin.
    pinPeripheral(10, PIO_SERCOM);
    pinPeripheral(11, PIO_SERCOM);
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

/*
  void platformSerial3Begin(int32_t baud) {
  Serial3.begin(baud);

  // Order is very important here. This has to happen after the call to begin.
  pinPeripheral(21, PIO_SERCOM);
  pinPeripheral(20, PIO_SERCOM);
  }
*/

#endif
