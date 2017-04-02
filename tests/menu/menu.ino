#include <SPI.h>
#include <Adafruit_GFX.h>
#include "Adafruit_STMPE610.h"
#include "Menu.h"

#ifdef HX8357
Adafruit_HX8357 tft(TFT_CS, TFT_DC, TFT_RST);
#else
Adafruit_ILI9341 tft(TFT_CS, TFT_DC);
#endif

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
