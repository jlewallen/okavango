#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "LoraRadio.h"
#include "Sniffer.h"
#include "core.h"

#include <stdio.h>

#ifdef ARDUINO_SAMD_FEATHER_M0
#define STMPE_CS 6
#define TFT_CS   9
#define TFT_DC   10
#define SD_CS    5
#endif

#ifdef ARDUINO_STM32_FEATHER
#define TFT_DC   PB4
#define TFT_CS   PA15
#define STMPE_CS PC7
#define SD_CS    PC5
#endif

#define PIN_RFM95_CS          8
#define PIN_RFM95_RST         4
#define PIN_RFM95_INT         3

class DisplayPacketOnTft : public PacketHandler {
private:
    Adafruit_ILI9341 *tft;
public:
    DisplayPacketOnTft(Adafruit_ILI9341 *tft) : tft(tft) {
    }

public:
    virtual void handle(rf95_header_t *header, fk_network_packet_t *packet, size_t packetSize) override {
        if (!fk_packet_is_control(packet)) {
            if (packet->kind == FK_PACKET_KIND_DATA_BOAT_SENSORS) {
                data_boat_packet_t db = { 0 };
                memcpy((uint8_t *)&db, (uint8_t *)packet, sizeof(data_boat_packet_t));

                Serial.println("Dispalying packet...");

                // tft->fillScreen(ILI9341_BLACK);
                tft->fillRect(0, 0, tft->width(), 100, ILI9341_BLACK);

                tft->setCursor(0, 0);
                tft->setTextSize(2);
                tft->setTextColor(ILI9341_WHITE);

                char buffer[256];
                snprintf(buffer, sizeof(buffer), "%d\n%f\n%f\n%f\n%f", db.time, db.latitude, db.longitude, db.altitude, db.speed);

                tft->println(buffer);
            } else {
                Serial.print("Ignoring packet: ");
                Serial.println(fk_packet_get_kind(packet));
            }
        }
        else {
            Serial.print("Ignoring control packet: ");
            Serial.println(fk_packet_get_kind(packet));
        }
    }

};

CorePlatform corePlatform;
Adafruit_ILI9341 tft(TFT_CS, TFT_DC);
RH_RF95 rf95;
LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST, PIN_RFM95_RST);
DisplayPacketOnTft handler(&tft);
Sniffer sniffer(&radio, &handler);
MillisSystemClock Clock;

void setup() {
    Serial.begin(115200);

    while (!Serial && millis() < 5000) {
        delay(10);
    }

    pinMode(5, OUTPUT);
    pinMode(6, OUTPUT);
    pinMode(9, OUTPUT);

    digitalWrite(5, HIGH);
    digitalWrite(6, HIGH);
    digitalWrite(9, HIGH);

    corePlatform.setup(5, PIN_RFM95_CS, PIN_RFM95_RST, false);

    if (!radio.setup()) {
        tft.fillScreen(ILI9341_RED);

        while (1) {

        }
    }

    tft.begin();

    Serial.println("Ready!");

    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0, 0);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_RED);
    tft.println("Ready!");
}

void loop(void) {
    while (1) {
        sniffer.tick();

        delay(50);
    }
}
