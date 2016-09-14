#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_HX8357.h"
#include "Adafruit_STMPE610.h"

#define TFT_CS               10
#define TFT_DC               9
#define TFT_RST              6

Adafruit_HX8357 tft(TFT_CS, TFT_DC, TFT_RST);
Adafruit_STMPE610 touch;

class Vector2 {
public:
    uint16_t x;
    uint16_t y;

public:
    Vector2() : x(0), y(0) {
    }

    Vector2(uint16_t x, uint16_t y) : x(x), y(y) {
    }

    Vector2 minus(const Vector2 &vector) const {
        return Vector2(x - vector.x, y - vector.y);
    }

    Vector2 plus(const Vector2 &vector) const {
        return Vector2(x + vector.x, y + vector.y);
    }
};

static inline Vector2 operator-(const Vector2 &lhs, const Vector2 &rhs) {
    return lhs.minus(rhs);
}

static inline Vector2 operator+(const Vector2 &lhs, const Vector2 &rhs) {
    return lhs.plus(rhs);
}

class Touch : public Vector2 {
public:
    uint8_t z;

public:
    Vector2 toPixelCoordinates(uint8_t rotation = 1) {
        Vector2 minimum(318, 235);
        Vector2 maximum(3815, 3859);
        Vector2 screen(480, 320);
        Vector2 size = maximum - minimum;
        Vector2 zeroed = *this - minimum;
        float scaledX = (zeroed.x / (float)size.x);
        float scaledY = (zeroed.y / (float)size.y);

        // Only using rotation 1 now.
        switch (rotation) {
        case 1:
            return Vector2((1.0 - scaledY) * screen.x, scaledX * screen.y);
        default:
            return Vector2(scaledX * screen.x, scaledY * screen.y);
        }
    }
};

class Rect {
public:
    Vector2 position;
    Vector2 size;

public:
    Rect(Vector2 position, Vector2 size) :
        position(position), size(size) {
    }

    Rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h) :
        position(Vector2(x, y)), size(Vector2(w, h)) {
    }

    bool contains(Vector2 p) {
        return p.x > (position.x) && p.x < (position.x + size.x) &&
               p.y > (position.y) && p.y < (position.y + size.y);
    }
};

class MenuOption {
private:
    const char *label;
    const uint8_t value;

public:
    MenuOption(const char *label, uint8_t value) :
        label(label), value(value) {
    }

public:
    const char *getLabel() const {
        return label;
    }

    const uint8_t getValue() const {
        return value;
    }
};

class Menu {
private:
    Adafruit_GFX *gfx;
    Adafruit_STMPE610 *touch;
    const MenuOption *options;
    const size_t numberOfOptions;
    bool visible = false;
    bool redraw = true;

public:
    template<size_t N>
    Menu(Adafruit_GFX *gfx, Adafruit_STMPE610 *touch, const MenuOption (&array)[N]) :
        gfx(gfx), touch(touch), options(array), numberOfOptions(N) {
    }

    void show() {
        if (!visible) {
            redraw = true;
        }
        visible = true;
    }

    void hide() {
        visible = false;
    }

    void tick() {
        if (visible) {
            if (redraw) {
                for (size_t i = 0; i < numberOfOptions; ++i) {
                    const MenuOption option = options[i];
                    Rect rect = getButtonRect(i);
                    drawButton(rect, option.getLabel());

                    Serial.print(rect.position.x);
                    Serial.print(", ");
                    Serial.print(rect.position.y);
                    Serial.print("  ");
                    Serial.print(rect.position.x + rect.size.x);
                    Serial.print(", ");
                    Serial.print(rect.position.y + rect.size.y);
                    Serial.println("");
                }
                redraw = false;
            }
            if (touch->touched()) {
                while (!touch->bufferEmpty()) {
                    Touch position;
                    touch->readData(&position.x, &position.y, &position.z);
                    Vector2 pixel = position.toPixelCoordinates();
                    const MenuOption *option = getOptionAtPixel(pixel);
                    if (option != nullptr) {
                        Serial.println(option->getLabel());
                    }
                    Serial.print(pixel.x);
                    Serial.print(", ");
                    Serial.print(pixel.y);
                    Serial.println("");
                }
                touch->writeRegister8(STMPE_INT_STA, 0xFF);
            }
        }
    }

private:
    const MenuOption *getOptionAtPixel(Vector2& position) {
        for (size_t i = 0; i < numberOfOptions; ++i) {
            Rect rect = getButtonRect(i);
            if (rect.contains(position)) {
                return &options[i];
            }
        }
        return nullptr;
    }
    void drawButton(Rect &rect, const char *label) {
        uint8_t textSize = 4;

        gfx->fillRect(rect.position.x, rect.position.y, rect.size.x, rect.size.y, HX8357_BLACK);
        gfx->drawRect(rect.position.x, rect.position.y, rect.size.x, rect.size.y, HX8357_WHITE);
        gfx->setCursor(rect.position.x + (rect.size.x / 2) - strlen(label) * 3 * textSize, rect.position.y + (rect.size.y / 2) - 4 * textSize);
        gfx->setTextColor(HX8357_WHITE);
        gfx->setTextSize(textSize);
        gfx->print(label);
    }

    Rect getButtonRect(size_t i) {
        const uint8_t numberOfRows = 2;
        const uint8_t numberOfColumns = 2;
        uint8_t row = i / numberOfColumns;
        uint8_t column = i % numberOfColumns;
        // Note: These are flipped due to the rotation of the TFT.
        uint16_t w = 480 / numberOfColumns;
        uint16_t h = 320 / numberOfRows;
        uint16_t x = w * column;
        uint16_t y = h * row;
        return Rect(x, y, w, h);
    }
};

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

    tft.begin(HX8357D);
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

    while (true) {
        menu.tick();
        delay(10);
    }
}

void loop(void) {

}
