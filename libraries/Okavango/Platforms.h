#ifndef PLATFORMS_H
#define PLATFORMS_H

#include <Arduino.h>

#ifdef ARDUINO_AVR_UNO

#include <SoftwareSerial.h>

#define PORT_EXPANDER_SELECT_PIN_0        6
#define PORT_EXPANDER_SELECT_PIN_1        7
#define PORT_EXPANDER_SELECT_PIN_2        8

#define PORT_EXPANDER_RX_PIN              2
#define PORT_EXPANDER_TX_PIN              3

#define CONDUCTIVITY_RX_PIN               4
#define CONDUCTIVITY_TX_PIN               5

#define OPEN_CONDUCTIVITY_SERIAL_ON_START true

#define RFM95_CS                          10
#define RFM95_RST                         9
#define RFM95_INT                         2

typedef SoftwareSerial SerialType;

extern SerialType portExpanderSerial;
extern SerialType conductivitySerial;

#endif /* ARDUINO_AVR_UNO */


#ifdef ARDUINO_SAMD_FEATHER_M0

#include "wiring_private.h" // pinPeripheral() function

#define PORT_EXPANDER_SELECT_PIN_0        5
#define PORT_EXPANDER_SELECT_PIN_1        6
#define PORT_EXPANDER_SELECT_PIN_2        9

#define WAIT_FOR_SERIAL                   1000 * 60

#define OPEN_CONDUCTIVITY_SERIAL_ON_START false

#define RFM95_CS                          19
#define RFM95_RST                         17
#define RFM95_INT                         18

typedef Uart SerialType;

extern SerialType &portExpanderSerial;
extern SerialType &conductivitySerial;

#endif /* ARDUINO_SAMD_FEATHER_M0 */

#include <SPI.h>
#include <RH_RF95.h>

void platformPostSetup();
void platformRestart();
float platformBatteryVoltage();

#endif
