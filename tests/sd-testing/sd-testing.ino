#include <Arduino.h>
#include <SD.h>
#include <wdt.h>

#include <SPI.h>
#include <RH_RF95.h>

#include "LoraRadio.h"

typedef struct fk_network_packet_t {
    uint8_t kind;
    char name[2];
} fk_network_packet_t;

typedef struct fk_network_ping_t {
    fk_network_packet_t fk;
    uint8_t batch;
} fk_network_ping_t;

#define PIN_FEATHER_M0_LORA_ADALOGGER_WING_RFM95_CS           8
#define PIN_FEATHER_M0_LORA_ADALOGGER_WING_RFM95_RST          4
#define PIN_FEATHER_M0_LORA_ADALOGGER_WING_RFM95_INT          3
#define PIN_FEATHER_M0_LORA_ADALOGGER_WING_SD_CS              16

#define PIN_RFM95_CS                                          PIN_FEATHER_M0_LORA_ADALOGGER_WING_RFM95_CS
#define PIN_RFM95_INT                                         PIN_FEATHER_M0_LORA_ADALOGGER_WING_RFM95_INT
#define PIN_RFM95_RST                                         PIN_FEATHER_M0_LORA_ADALOGGER_WING_RFM95_RST
#define PIN_SD_CS                                             PIN_FEATHER_M0_LORA_ADALOGGER_WING_SD_CS
#define PIN_LED                                               13

#define PIN_MODE_SELECTOR                                     6

#define DIE_SD_OPEN                                           4
#define DIE_SD_WRITE                                          5
#define DIE_RFM_BEGIN                                         6
#define DIE_RFM_SEND                                          7

void unblink_led(uint8_t times) {
    for (uint8_t i = 0; i < times; ++i) {
        digitalWrite(PIN_LED, LOW);
        delay(250);
        digitalWrite(PIN_LED, HIGH);
        delay(250);
    }
}

void blink_led(uint8_t times) {
    for (uint8_t i = 0; i < times; ++i) {
        digitalWrite(PIN_LED, HIGH);
        delay(250);
        digitalWrite(PIN_LED, LOW);
        delay(250);
    }
}

void die(uint8_t ledFlashes, bool restart = false) {
    while (true) {
        blink_led(ledFlashes);
        delay(1000);
        if (!restart) {
            wdt_reset();
        }
    }
}

void writeToSdAndListen() {
    LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST, PIN_RFM95_RST);

    uint8_t error = SD.beginWithDetailedError(PIN_SD_CS);
    if (error > 0) {
        die(error);
    }

    wdt_reset();

    delay(random(100, 1000));

    if (radio.setup()) {
        radio.tick();
    }
    else {
        die(DIE_RFM_BEGIN);
    }

    File file = SD.open("TEMP.DAT", FILE_WRITE);
    if (!file) {
        die(DIE_SD_OPEN);
    }

    digitalWrite(PIN_LED, HIGH);

    file.seek(0);

    while (true) {
        uint8_t buffer[128];
        memset(&buffer, 0, sizeof(buffer));
        if (file.write((uint8_t *)&buffer, sizeof(buffer)) != sizeof(buffer)) {
            die(DIE_SD_WRITE, true);
        }
        file.flush();

        digitalWrite(PIN_LED, LOW);
        delay(25);
        digitalWrite(PIN_LED, HIGH);

        radio.tick();

        if (radio.hasPacket()) {
            radio.clear();
            Serial.println("PACKET!");
            unblink_led(1);
        }

        Serial.print(".");
    }
}

void sendPings() {
    LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST, PIN_RFM95_RST);

    if (radio.setup()) {
        radio.sleep();
    }
    else {
        die(DIE_RFM_BEGIN);
    }

    while (true) {
        fk_network_ping_t packet;
        memset((uint8_t *)&packet, 0, sizeof(fk_network_ping_t));
        if (!radio.send((uint8_t *)&packet, sizeof(fk_network_ping_t))) {
            die(DIE_RFM_SEND);
        }
        radio.waitPacketSent();
        radio.sleep();

        blink_led(1);

        delay(5000);

        wdt_reset();
    }
}

void setup() {
    pinMode(PIN_MODE_SELECTOR, INPUT);

    Serial.begin(115200);

    while (millis() < 1000 * 2 && !Serial) {
        delay(100);
    }

    Serial.println(sizeof(fk_network_ping_t));

    if (!Serial) {
        wdt_enable(WDT_PERIOD_8X);
    }

    randomSeed(analogRead(A0));

    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    pinMode(PIN_SD_CS, OUTPUT);
    digitalWrite(PIN_SD_CS, HIGH);

    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH);

    pinMode(PIN_RFM95_CS, OUTPUT);
    digitalWrite(PIN_RFM95_CS, HIGH);

    pinMode(PIN_RFM95_RST, OUTPUT);
    digitalWrite(PIN_RFM95_RST, HIGH);

    if (digitalRead(PIN_MODE_SELECTOR)) {
        writeToSdAndListen();
    }
    else {
        sendPings();
    }
}

void loop() {

}
