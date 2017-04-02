#ifndef MENU_H_INCLUDED
#define MENU_H_INCLUDED

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_STMPE610.h>

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
#endif

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

class Touch : public Vector2 {
public:
    uint8_t z;

public:
    Vector2 toPixelCoordinates(uint8_t rotation = 1);

};

class MenuOption {
private:
    const char *label;
    const uint8_t value;
    bool redraw = false;
    bool pressing = false;
    uint32_t pressedAt = 0;

public:
    MenuOption(const char *label, uint8_t value);

public:
    const char *getLabel() const;
    const uint8_t getValue() const;
    bool redrawNecessary();
    void redrawNecessary(bool necessary);
    void draw(Adafruit_GFX *gfx, const Rect &rect);
    bool touched();
    bool untouch();

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
    void show();
    void hide();
    void tick();

private:
    MenuOption *getOptionAtPixel(Vector2& position);
    void untouch();
    Rect getButtonRect(size_t i);
};

#endif
