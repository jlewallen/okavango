#ifndef PLATFORMS_H
#define PLATFORMS_H

#include <Arduino.h>

#define FEATHER_WING_ADALOGGER

// Primarily for Feather FONA with the Adalogger wing, though we
// could use the Lora version too.
#ifdef ARDUINO_AVR_FEATHER32U4

#include <SoftwareSerial.h>

#define FEATHER_DISABLE_RTC

#define PIN_FEATHER_32U4_EXTERNAL_LORA_RFM95_CS              23
#define PIN_FEATHER_32U4_EXTERNAL_LORA_RFM95_RST             21
#define PIN_FEATHER_32U4_EXTERNAL_LORA_RFM95_INT             2
#define PIN_FEATHER_32U4_ADALOGGER_WING_SD_CS                20

#define PIN_RFM95_CS                                         PIN_FEATHER_32U4_EXTERNAL_LORA_RFM95_CS
#define PIN_RFM95_RST                                        PIN_FEATHER_32U4_EXTERNAL_LORA_RFM95_RST
#define PIN_RFM95_INT                                        PIN_FEATHER_32U4_EXTERNAL_LORA_RFM95_INT
#define PIN_SD_CS                                            PIN_FEATHER_32U4_ADALOGGER_WING_SD_CS

#define PIN_RED_LED                                          13
#define PIN_GREEN_LED                                        13
#define PIN_SLEEP_LED                                        13

#define PIN_ROCK_BLOCK                                       -1

typedef SoftwareSerial SerialType;

#endif

// Simple/basic testing on an Ardunio. Many things won't work.
#ifdef ARDUINO_AVR_UNO

#include <SoftwareSerial.h>

#define PORT_EXPANDER_SELECT_PIN_0                           6
#define PORT_EXPANDER_SELECT_PIN_1                           7

#define PORT_EXPANDER_RX_PIN                                 2
#define PORT_EXPANDER_TX_PIN                                 3

#define CONDUCTIVITY_RX_PIN                                  4
#define CONDUCTIVITY_TX_PIN                                  5

#define OPEN_CONDUCTIVITY_SERIAL_ON_START                    true

#define PIN_ARDUINO_UNO_EXTERNAL_LORA_RFM95_CS               10
#define PIN_ARDUINO_UNO_EXTERNAL_LORA_RFM95_RST              9
#define PIN_ARDUINO_UNO_EXTERNAL_LORA_RFM95_INT              2

#define PIN_RED_LED                                          13
#define PIN_GREEN_LED                                        13
#define PIN_SLEEP_LED                                        13

typedef SoftwareSerial SerialType;

extern SerialType portExpanderSerial;
extern SerialType conductivitySerial;

#endif /* ARDUINO_AVR_UNO */


// Our core platform, at least hopefully eventually.
#ifdef ARDUINO_SAMD_FEATHER_M0

#include "wiring_private.h" // pinPeripheral() function

#define PORT_EXPANDER_SELECT_PIN_0                           5
#define PORT_EXPANDER_SELECT_PIN_1                           6

#define OPEN_CONDUCTIVITY_SERIAL_ON_START                    false

#define PIN_FEATHER_M0_LORA_ADALOGGER_WING_RFM95_CS          8
#define PIN_FEATHER_M0_LORA_ADALOGGER_WING_RFM95_RST         4
#define PIN_FEATHER_M0_LORA_ADALOGGER_WING_RFM95_INT         3
#define PIN_FEATHER_M0_LORA_ADALOGGER_WING_SD_CS             16

#define PIN_FEATHER_M0_ADALOGGER_EXTERNAL_LORA_RFM95_CS      19
#define PIN_FEATHER_M0_ADALOGGER_EXTERNAL_LORA_RFM95_RST     17
#define PIN_FEATHER_M0_ADALOGGER_EXTERNAL_LORA_RFM95_INT     18
#define PIN_FEATHER_M0_ADALOGGER_EXTERNAL_LORA_SD_CS         4

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

#define PIN_RED_LED                                          13
#define PIN_GREEN_LED                                        8
#define PIN_SLEEP_LED                                        8

#define PIN_ROCK_BLOCK                                       9
#define PIN_WEATHER_STATION_RESET                            6
#define PIN_DS18B20                                          19

#define HAVE_DHT22
#define PIN_DHT                                              18

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
#define LOW_POWER_SLEEP_BEGIN                                0
#define WAIT_FOR_SERIAL                                      1000 * 10

#define RF95_FREQ                                            915.0

#define FK_SETTINGS_QUEUE_FILENAME                           "queue.bin"
#define FK_SETTINGS_WEATHER_STATION_DATA_FILENAME            "weather.csv"
#define FK_SETTINGS_ATLAS_DATA_FILENAME                      "atlas.csv"
#define FK_SETTINGS_DATA_BOAT_DATA_FILENAME                  "db.csv"
#define FK_SETTINGS_TRANSMISSION_STATUS_FILENAME             "status.bin"
#define FK_SETTINGS_CONFIGURATION_FILENAME                   "fk.cfg"
#define FK_SETTINGS_BACKUP_DATA_FILENAME                     "backup.bin"
#define FK_SETTINGS_TRANSMISSION_INTERVAL                    1 * 60 * 1000

#define TRANSMISSION_KIND_LOCATION  0
#define TRANSMISSION_KIND_SENSORS   1
#define TRANSMISSION_KIND_KINDS     2

extern uint32_t TransmissionIntervals[];

class LogPrinter : public Stream {
public:
    bool open();

public:
    virtual int available() override;
    virtual int read() override;
    virtual int peek() override;
    virtual void flush() override;
    virtual size_t write(uint8_t) override;
    virtual size_t write(const uint8_t *buffer, size_t size) override;
};

extern LogPrinter logPrinter;

#define DEBUG_PRINTLN(value)                                 logPrinter.println(value)
#define DEBUG_PRINT(value)                                   logPrinter.print(value)

#define memzero(ptr, sz)                                     memset(ptr, 0, sz)

void platformPostSetup();
void platformRestart();
float platformBatteryVoltage();
void platformCatastrophe(uint8_t pin);
void platformLowPowerSleep(uint32_t numberOfMs);
void platformBlink(uint8_t pin);
uint32_t platformFreeMemory();

typedef enum ConductivityConfig {
    None,
    OnSerial2,
    OnExpanderPort4
} ConductivityConfig;

#define FIVE_MINUTES                                         (1000 * 60 * 5)
#define THIRTY_MINUTES                                       (60 * 1000 * 30)
#define ONE_MINUTE                                           (60 * 1000)

#endif
