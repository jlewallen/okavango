#include "Menu.h"

Vector2 Touch::toPixelCoordinates(uint8_t rotation) {
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

MenuOption::MenuOption(const char *label, uint8_t value) :
    label(label), value(value) {
}

const char *MenuOption::getLabel() const {
    return label;
}

const uint8_t MenuOption::getValue() const {
    return value;
}

bool MenuOption::redrawNecessary() {
    return redraw;
}

void MenuOption::redrawNecessary(bool necessary) {
    redraw = necessary;
}

void MenuOption::draw(Adafruit_GFX *gfx, const Rect &rect) {
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

bool MenuOption::touched() {
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

bool MenuOption::untouch() {
    if (pressing) {
        Serial.print(label);
        Serial.println(": untouched, redrawing");
        redraw = true;
    }
    pressing = false;
    return redraw;
}

void Menu::show() {
    if (!visible) {
        for (size_t i = 0; i < numberOfOptions; ++i) {
            options[i].redrawNecessary(true);
        }
    }
    visible = true;
}

void Menu::hide() {
    visible = false;
}

void Menu::tick() {
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

MenuOption *Menu::getOptionAtPixel(Vector2& position) {
    for (size_t i = 0; i < numberOfOptions; ++i) {
        Rect rect = getButtonRect(i);
        if (rect.contains(position)) {
            return &options[i];
        }
    }
    return nullptr;
}

void Menu::untouch() {
    for (size_t i = 0; i < numberOfOptions; ++i) {
        options[i].untouch();
    }
}

Rect Menu::getButtonRect(size_t i) {
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
