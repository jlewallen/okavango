#ifndef PLATFORMS_H
#define PLATFORMS_H

#include <Arduino.h>

#ifdef ARDUINO_AVR_UNO

#include <SoftwareSerial.h>

#define PORT_EXPANDER_SELECT_PIN_0        6
#define PORT_EXPANDER_SELECT_PIN_1        7

#define PORT_EXPANDER_RX_PIN              2
#define PORT_EXPANDER_TX_PIN              3

#define CONDUCTIVITY_RX_PIN               4
#define CONDUCTIVITY_TX_PIN               5

#define OPEN_CONDUCTIVITY_SERIAL_ON_START true

#define RFM95_CS                          10
#define RFM95_RST                         9
#define RFM95_INT                         2
#define PIN_SD_CS                         ?
#define PIN_DS18B20                       ?

#define PIN_RED_LED                       13
#define PIN_GREEN_LED                     13
#define PIN_SLEEP_LED                     13

typedef SoftwareSerial SerialType;

extern SerialType portExpanderSerial;
extern SerialType conductivitySerial;

#endif /* ARDUINO_AVR_UNO */


#ifdef ARDUINO_SAMD_FEATHER_M0

#include "wiring_private.h" // pinPeripheral() function

#define PORT_EXPANDER_SELECT_PIN_0        5
#define PORT_EXPANDER_SELECT_PIN_1        6

#define WAIT_FOR_SERIAL                   1000 * 30

#define OPEN_CONDUCTIVITY_SERIAL_ON_START false

#ifdef FEATHER_WING
# define RFM95_CS                         8
# define RFM95_RST                        4
# define RFM95_INT                        3
# define PIN_SD_CS                        10
#else
# define RFM95_CS                         19
# define RFM95_RST                        17
# define RFM95_INT                        18
# define PIN_SD_CS                        4
#endif

#define PIN_DS18B20                       14

#define PIN_RED_LED                       13
#define PIN_GREEN_LED                     8
#define PIN_SLEEP_LED                     8

typedef Uart SerialType;

extern SerialType &portExpanderSerial;
extern SerialType &conductivitySerial;

#endif /* ARDUINO_SAMD_FEATHER_M0 */

#include <SPI.h>
#include <RH_RF95.h>

void platformPostSetup();
void platformRestart();
float platformBatteryVoltage();
void platformCatastrophe(uint8_t pin);
void platformLowPowerSleep(uint32_t numberOfMs);
void platformBlink(uint8_t pin);

#define LOW_POWER_SLEEP_END               1000 * 60 * 5
#define LOW_POWER_SLEEP_BEGIN             0

#endif
