#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GPS.h>
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

#define DEGREES_TO_RADIANS(d) ((d) * (PI / 180.0f))

class DisplayPacketOnTft : public PacketHandler {
private:
    data_boat_packet_t db;
    Adafruit_ILI9341 *tft;
    LoraRadio *radio;
    Adafruit_GPS *gps;

public:
    DisplayPacketOnTft(Adafruit_ILI9341 *tft, LoraRadio *radio, Adafruit_GPS *gps) : tft(tft), radio(radio), gps(gps) {
        memset((uint8_t *)&db, 0, sizeof(data_boat_packet_t));
    }

private:
    float calculateDistance(float lat1, float lon1, float lat2, float lon2) {
        float earthsRadius = 6371.0f;
        float dLat = DEGREES_TO_RADIANS(lat2 - lat1);
        float dLon = DEGREES_TO_RADIANS(lon2 - lon1);
        float a =
            sin(dLat / 2.0f) * sin(dLat / 2.0f) +
            cos(DEGREES_TO_RADIANS(lat1)) * cos(DEGREES_TO_RADIANS(lat2)) *
            sin(dLon / 2.0f) * sin(dLon / 2.0f);
        float c = 2.0f * atan2(sqrt(a), sqrt(1 - a));
        float d = earthsRadius * c;

        /*
          Serial.print(dLat);
          Serial.print(" ");
          Serial.print(dLon);
          Serial.print(" ");
          Serial.print(a);
          Serial.print(" ");
          Serial.print(c);
          Serial.print(" ");
          Serial.print(d);
          Serial.println("");
        */

        return d;
    }

    void secondsToDuration(int16_t secs, char *buffer, size_t len) {
        if (secs < 60) {
            snprintf(buffer, len, "%ds ago", secs);
        }
        else if (secs < 60 * 60) {
            snprintf(buffer, len, "%.2fm ago", secs / 60.0f);
        }
        else {
            snprintf(buffer, len, "%.2fh ago", secs / 60.0f / 60.0f);
        }
    }

public:
    void drawLocalInformation(bool indicator) {
        tft->fillRect(0, 150, tft->width(), 150, ILI9341_WHITE);
        tft->setCursor(0, 155);
        tft->setTextSize(2);
        tft->setTextColor(ILI9341_BLACK);

        char buffer[256];

        DateTime now = DateTime(gps->year, gps->month, gps->day, gps->hour, gps->minute, gps->seconds);
        int16_t age = now.unixtime() - db.time;
        char prettyAge[32];
        secondsToDuration(age, prettyAge, sizeof(prettyAge));

        snprintf(buffer, sizeof(buffer), " %d/%d %02d:%02d:%02d\n\n %f\n %f\n A: %f\n S: %f\n D: %f\n Age: %s",
                 gps->month, gps->day, gps->hour, gps->minute, gps->seconds,
                 gps->latitudeDegrees, gps->longitudeDegrees, gps->altitude, gps->speed,
                 calculateDistance(gps->latitudeDegrees, gps->longitudeDegrees, db.latitude, db.longitude),
                 prettyAge
            );
        tft->println(buffer);

        tft->fillCircle(230, 310, 4, indicator ? (gps->fix ? ILI9341_WHITE : ILI9341_RED) : ILI9341_BLACK);
    }

    virtual void handle(rf95_header_t *header, fk_network_packet_t *packet, size_t packetSize) override {
        if (!fk_packet_is_control(packet)) {
            if (packet->kind == FK_PACKET_KIND_DATA_BOAT_SENSORS) {
                radio->sleep();

                memcpy((uint8_t *)&db, (uint8_t *)packet, sizeof(data_boat_packet_t));

                Serial.println("Dispalying packet...");
                Serial.print("Values:");

                // pH, DO, Temp, ORP, EC
                for (size_t i = 0; i < FK_ATLAS_SENSORS_PACKET_NUMBER_VALUES; ++i) {
                    Serial.print(" ");
                    Serial.print(db.values[i]);
                }

                Serial.println("");

                tft->fillRect(0, 0, tft->width(), 150, ILI9341_BLACK);
                tft->setCursor(0, 0);
                tft->setTextSize(2);
                tft->setTextColor(ILI9341_WHITE);

                char buffer[256];

                DateTime dateTime = DateTime(db.time);
                snprintf(buffer, sizeof(buffer), " %d/%d %02d:%02d:%02d\n\n %f\n %f\n A: %f\n S: %f\n B: %d%%",
                         dateTime.month(), dateTime.day(), dateTime.hour(), dateTime.minute(), dateTime.second(),
                         db.latitude, db.longitude, db.altitude, db.speed, (int32_t)db.battery);
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
Adafruit_GPS gps(&Serial1);
DisplayPacketOnTft handler(&tft, &radio, &gps);
Sniffer sniffer(&radio, &handler);
MillisSystemClock Clock;

void setup() {
    Serial.begin(115200);
    Serial1.begin(9600);

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
    bool indicator = false;
    uint32_t tick = millis();

    while (1) {
        while (Serial1.available()) {
            char c = gps.read();
        }

        if (gps.newNMEAreceived()) {
            if (gps.parse(gps.lastNMEA())) {
            }
        }

        if (millis() - tick > 1000) {
            radio.idle();
            handler.drawLocalInformation(indicator);
            indicator = !indicator;
            tick = millis();
        }

        sniffer.tick();

        delay(10);
    }
}
