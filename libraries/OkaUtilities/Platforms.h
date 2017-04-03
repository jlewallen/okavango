#ifndef PLATFORMS_H
#define PLATFORMS_H

#include <Arduino.h>

#define FEATHER_WING_ADALOGGER

#ifdef ARDUINO_SAMD_FEATHER_M0

#include "wiring_private.h" // pinPeripheral() function

#define PORT_EXPANDER_SELECT_PIN_0                           5
#define PORT_EXPANDER_SELECT_PIN_1                           6

#define OPEN_CONDUCTIVITY_SERIAL_ON_START                    false

#define PIN_FEATHER_M0_ADALOGGER_LORA_WING_RFM95_CS          19
#define PIN_FEATHER_M0_ADALOGGER_LORA_WING_RFM95_RST         17
#define PIN_FEATHER_M0_ADALOGGER_LORA_WING_RFM95_INT         18
#define PIN_FEATHER_M0_ADALOGGER_LORA_WING_SD_CS             4

#define PIN_FEATHER_M0_LORA_ADALOGGER_WING_RFM95_CS          8
#define PIN_FEATHER_M0_LORA_ADALOGGER_WING_RFM95_RST         4
#define PIN_FEATHER_M0_LORA_ADALOGGER_WING_RFM95_INT         3
#define PIN_FEATHER_M0_LORA_ADALOGGER_WING_SD_CS             16

#define PIN_FEATHER_M0_ADALOGGER_EXTERNAL_LORA_RFM95_CS      19
#define PIN_FEATHER_M0_ADALOGGER_EXTERNAL_LORA_RFM95_RST     17
#define PIN_FEATHER_M0_ADALOGGER_EXTERNAL_LORA_RFM95_INT     18
#define PIN_FEATHER_M0_ADALOGGER_EXTERNAL_LORA_SD_CS         4

#define PIN_ULTRASONIC_SENSOR                                A0

#ifdef FEATHER_WING_LORA
#define PIN_RFM95_CS                                         PIN_FEATHER_M0_ADALOGGER_LORA_WING_RFM95_CS
#define PIN_RFM95_RST                                        PIN_FEATHER_M0_ADALOGGER_LORA_WING_RFM95_RST
#define PIN_RFM95_INT                                        PIN_FEATHER_M0_ADALOGGER_LORA_WING_RFM95_INT
#define PIN_SD_CS                                            PIN_FEATHER_M0_ADALOGGER_LORA_WING_SD_CS
#else
#ifdef FEATHER_WING_ADALOGGER
#define PIN_RFM95_CS                                         PIN_FEATHER_M0_LORA_ADALOGGER_WING_RFM95_CS
#define PIN_RFM95_RST                                        PIN_FEATHER_M0_LORA_ADALOGGER_WING_RFM95_RST
#define PIN_RFM95_INT                                        PIN_FEATHER_M0_LORA_ADALOGGER_WING_RFM95_INT
#define PIN_SD_CS                                            PIN_FEATHER_M0_LORA_ADALOGGER_WING_SD_CS
#else
#define PIN_RFM95_CS                                         PIN_FEATHER_M0_ADALOGGER_EXTERNAL_LORA_RFM95_CS
#define PIN_RFM95_RST                                        PIN_FEATHER_M0_ADALOGGER_EXTERNAL_LORA_RFM95_RST
#define PIN_RFM95_INT                                        PIN_FEATHER_M0_ADALOGGER_EXTERNAL_LORA_RFM95_INT
#define PIN_SD_CS                                            PIN_FEATHER_M0_ADALOGGER_EXTERNAL_LORA_SD_CS
#endif
#endif

#define PIN_RED_LED                                          13
#define PIN_GREEN_LED                                        8
#define PIN_SLEEP_LED                                        8

#define PIN_ROCK_BLOCK                                       18
#define PIN_WEATHER_STATION_RESET                            19
#define PIN_POWER_HARD_RESET                                 5

extern Uart Serial2;

typedef Uart SerialType;

extern SerialType &portExpanderSerial;
extern SerialType &conductivitySerial;

extern void platformSerial2Begin(int32_t baud);

#define F(text)                                              text

#endif /* ARDUINO_SAMD_FEATHER_M0 */

#include <SPI.h>
#include <RH_RF95.h>

#define LOW_POWER_SLEEP_DATA_BOAT_END                        (1000 * 30 * 5)
#define LOW_POWER_SLEEP_SENSORS_END                          (1000 * 60 * 5 * 4)
#define WAIT_FOR_SERIAL                                      (1000 * 10)

#define RF95_FREQ                                            915.0

#define FK_SETTINGS_QUEUE_FILENAME                           "queue.bin"
#define FK_SETTINGS_WEATHER_STATION_DATA_FILENAME            "weather.csv"
#define FK_SETTINGS_ATLAS_DATA_FILENAME                      "atlas.csv"
#define FK_SETTINGS_SONAR_DATA_FILENAME                      "sonar.csv"
#define FK_SETTINGS_DATA_BOAT_DATA_FILENAME                  "db.csv"
#define FK_SETTINGS_TRANSMISSION_STATUS_FILENAME             "status.bin"
#define FK_SETTINGS_CONFIGURATION_FILENAME                   "fk.cfg"
#define FK_SETTINGS_BACKUP_DATA_FILENAME                     "backup.bin"
#define FK_SETTINGS_TRANSMISSION_INTERVAL                    1 * 60 * 1000

#define DEBUG_PRINTLN(value)                                 logPrinter.println(value)
#define DEBUG_PRINT(value)                                   logPrinter.print(value)

#define memzero(ptr, sz)                                     memset(ptr, 0, sz)

#define PLATFORM_CATASTROPHE_FAST_BLINK                      2
#define PLATFORM_CATASTROPHE_PULSE                           1

void platformPostSetup();
void platformRestart();
float platformBatteryVoltage();
float platformBatteryLevel();
void platformCatastrophe(uint8_t pin, uint8_t mode = PLATFORM_CATASTROPHE_PULSE);
void platformBlink(uint8_t pin);
void platformBlinks(uint8_t pin, uint8_t number);
uint32_t platformFreeMemory();
uint32_t platformDeepSleep(bool forceDelay);
uint32_t platformUptime();
uint32_t platformAdjustUptime(uint32_t by);

typedef enum ConductivityConfig {
    None,
    OnSerial2,
    OnExpanderPort4
} ConductivityConfig;

#define FIVE_MINUTES                                         (1000 * 60 * 5)
#define THIRTY_MINUTES                                       (60 * 1000 * 30)
#define ONE_MINUTE                                           (60 * 1000)

#define weatherSerialBegin()   Serial1.begin(9600)
#define WeatherSerial          Serial1

#define rockBlockSerialBegin() platformSerial2Begin(19200)
#define RockBlockSerial        Serial2

#include "LogPrinter.h"

#endif
