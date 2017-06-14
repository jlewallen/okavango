#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "LoraRadio.h"
#include "Sniffer.h"
#include "core.h"

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
        Serial.println("Dispalying packet...");

        tft->fillScreen(ILI9341_BLACK);
    }

};

Adafruit_ILI9341 tft(TFT_CS, TFT_DC);
RH_RF95 rf95;
LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST, PIN_RFM95_RST);
DisplayPacketOnTft handler(&tft);
Sniffer sniffer(&radio, &handler);
MillisSystemClock Clock;

void setup() {
    Serial.begin(115200);
    delay(10);

    tft.begin();

    if (!radio.setup()) {
        tft.fillScreen(ILI9341_RED);

        while (1) {

        }
    }

    while (!Serial && millis() < 5000) {
        delay(10);
    }

    Serial.println("Ready!");

    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0, 0);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_GREEN);
    tft.println("Ready!");
}

void loop(void) {
    while (1) {
        sniffer.tick();

        delay(50);
    }
}
