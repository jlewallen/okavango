#include <SPI.h>
#include <Adafruit_GFX.h>
#include "Adafruit_STMPE610.h"

#define TOUCH_HYSTERESIS     100

#ifdef HX8357

#include <Adafruit_HX8357.h>

#define TFT_CS               10
#define TFT_DC               9
#define TFT_RST              6

#define COLOR_BLACK          HX8357_BLACK
#define COLOR_WHITE          HX8357_WHITE
#define SCREEN_WIDTH         320
#define SCREEN_HEIGHT        480

#define TS_MINX              318
#define TS_MINY              235
#define TS_MAXX              3815
#define TS_MAXY              3859

Adafruit_HX8357 tft(TFT_CS, TFT_DC, TFT_RST);

#else

#include <Adafruit_ILI9341.h>

#define TFT_CS               9
#define TFT_DC               10
#define STMPE_CS             6
#define SD_CS                5

#define COLOR_BLACK          ILI9341_BLACK
#define COLOR_WHITE          ILI9341_WHITE
#define SCREEN_WIDTH         320
#define SCREEN_HEIGHT        240

#define TS_MINX              150
#define TS_MINY              130
#define TS_MAXX              3800
#define TS_MAXY              4000

Adafruit_ILI9341 tft(TFT_CS, TFT_DC);

#endif

#include "Menu.h"

Adafruit_STMPE610 touch(STMPE_CS);

void setup() {
    Serial.begin(115200);

    uint32_t started = millis();
    while (!Serial && millis() - started < 1000 * 5) {

    }

    Serial.println("Begin");

    if (!touch.begin()) {
        Serial.println("STMPE missing!");
        while(1);
    }

    #ifdef HX8357
    tft.begin(HX8357D);
    #else
    tft.begin();
    #endif
    tft.setRotation(1);

    MenuOption options[] = {
        MenuOption("Help", 1),
        MenuOption("Test", 2),
        MenuOption("Exit", 3),
        MenuOption("Send", 4)
    };
    Menu menu((Adafruit_GFX *)&tft, &touch, options);
    menu.show();

    Serial.println("Showing");

    uint32_t tickAt = millis();

    while (true) {
        menu.tick();
        delay(10);

        if (millis() - tickAt > 2000) {
            tickAt = millis();
            Serial.println();
            Serial.println();
        }
    }
}

void loop(void) {

}
