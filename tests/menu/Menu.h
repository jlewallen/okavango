#ifndef MENU_H_INCLUDED
#define MENU_H_INCLUDED

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
        Vector2 minimum(TS_MINX, TS_MINY);
        Vector2 maximum(TS_MAXX, TS_MAXY);
        Vector2 screen(SCREEN_WIDTH, SCREEN_HEIGHT);
        Vector2 size = maximum - minimum;
        Vector2 zeroed = *this - minimum;
        float scaledX = (zeroed.x / (float)size.x);
        float scaledY = (zeroed.y / (float)size.y);

        // Only using rotation 1 now.
        switch (rotation) {
        case 1:
            #ifdef HX8357
            return Vector2((1.0 - scaledY) * screen.x, scaledX * screen.y);
            #else
            return Vector2(scaledY * screen.x, scaledX * screen.y);
            #endif
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
    bool redraw = false;
    bool pressing = false;
    uint32_t pressedAt = 0;

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

    bool redrawNecessary() {
        return redraw;
    }

    void redrawNecessary(bool necessary) {
        redraw = necessary;
    }

    void draw(Adafruit_GFX *gfx, const Rect &rect) {
        uint8_t textSize = 4;

        uint16_t fore = pressing ? COLOR_BLACK : COLOR_WHITE;
        uint16_t back = pressing ? COLOR_WHITE : COLOR_BLACK;

        gfx->fillRect(rect.position.x, rect.position.y, rect.size.x, rect.size.y, back);
        gfx->drawRect(rect.position.x, rect.position.y, rect.size.x, rect.size.y, fore);
        gfx->setCursor(rect.position.x + (rect.size.x / 2) - strlen(label) * 3 * textSize, rect.position.y + (rect.size.y / 2) - 4 * textSize);
        gfx->setTextColor(fore);
        gfx->setTextSize(textSize);
        gfx->print(label);

        redraw = false;
    }

    bool touched() {
        if (pressedAt > 0 && millis() - pressedAt > TOUCH_HYSTERESIS) {
            if (!pressing) {
                Serial.print(label);
                Serial.println(": touched, redrawing");
                redraw = true;
                pressedAt = 0;
            }
            pressing = true;
        }
        else if (pressedAt == 0) {
            pressedAt = millis();
        }
        return redraw;
    }

    bool untouch() {
        if (pressing) {
            Serial.print(label);
            Serial.println(": untouched, redrawing");
            redraw = true;
        }
        pressing = false;
        return redraw;
    }

};

class Menu {
private:
    Adafruit_GFX *gfx;
    Adafruit_STMPE610 *touch;
    MenuOption *options;
    const size_t numberOfOptions;
    bool visible = false;

public:
    template<size_t N>
    Menu(Adafruit_GFX *gfx, Adafruit_STMPE610 *touch, MenuOption (&array)[N]) :
        gfx(gfx), touch(touch), options(array), numberOfOptions(N) {
    }

    void show() {
        if (!visible) {
            for (size_t i = 0; i < numberOfOptions; ++i) {
                options[i].redrawNecessary(true);
            }
        }
        visible = true;
    }

    void hide() {
        visible = false;
    }

    void tick() {
        if (visible) {
            for (size_t i = 0; i < numberOfOptions; ++i) {
                if (options[i].redrawNecessary()) {
                    Rect rect = getButtonRect(i);
                    options[i].draw(gfx, rect);

                    Serial.print(options[i].getLabel());
                    Serial.print(": ");
                    Serial.print(rect.position.x);
                    Serial.print(", ");
                    Serial.print(rect.position.y);
                    Serial.print("  ");
                    Serial.print(rect.position.x + rect.size.x);
                    Serial.print(", ");
                    Serial.print(rect.position.y + rect.size.y);
                    Serial.println("");
                }
            }
            if (touch->touched()) {
                while (!touch->bufferEmpty()) {
                    Touch position;
                    touch->readData(&position.x, &position.y, &position.z);
                    Vector2 pixel = position.toPixelCoordinates();
                    MenuOption *option = getOptionAtPixel(pixel);
                    if (option != nullptr) {
                        option->touched();
                        /*
                        Serial.print(option->getLabel());
                        Serial.print(": ");
                        Serial.print(pixel.x);
                        Serial.print(", ");
                        Serial.print(pixel.y);
                        Serial.println("");
                        */
                    }
                }
                touch->writeRegister8(STMPE_INT_STA, 0xFF);
            }
            else {
                if (touch->bufferSize() > 0) {
                    Serial.println("Clearing STMPE610...");
                    while (!touch->bufferEmpty()) {
                        Touch position;
                        touch->readData(&position.x, &position.y, &position.z);
                    }
                    touch->writeRegister8(STMPE_INT_STA, 0xFF);
                }
                untouch();
            }
        }
    }

private:
    MenuOption *getOptionAtPixel(Vector2& position) {
        for (size_t i = 0; i < numberOfOptions; ++i) {
            Rect rect = getButtonRect(i);
            if (rect.contains(position)) {
                return &options[i];
            }
        }
        return nullptr;
    }

    void untouch() {
        for (size_t i = 0; i < numberOfOptions; ++i) {
            options[i].untouch();
        }
    }

    Rect getButtonRect(size_t i) {
        const uint8_t numberOfRows = 2;
        const uint8_t numberOfColumns = 2;
        uint8_t row = i / numberOfColumns;
        uint8_t column = i % numberOfColumns;
        uint16_t w = SCREEN_WIDTH / numberOfColumns;
        uint16_t h = SCREEN_HEIGHT / numberOfRows;
        uint16_t x = w * column;
        uint16_t y = h * row;
        return Rect(x, y, w, h);
    }
};

#endif
