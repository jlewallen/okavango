#include <SPI.h>
#include <Wire.h>

#include "Adafruit_STMPE610.h"

Adafruit_STMPE610 touch = Adafruit_STMPE610();

/*
 *     My 3.5" screen:
 *
 *     +-------------------------------------------------------+
 *     |(318,3859)                                    (318,235)|
 *     |                                                       |
 *     |                                                       |
 *     |                                                       |
 *     |                                                       |
 *     |                                                       |
 *     |                                                       |
 *     |                                                       |
 *     |(3815,3859)                                  (3815,235)|
 * GND +-------------------------------------------------------+ GND
 *
 * Min Z = 13
 * Max Z = 185
 *
*/

typedef struct touch_position_t {
    uint16_t x;
    uint16_t y;
    uint8_t z;
} touch_position_t;

touch_position_t min = { 65535, 65535, 255 };
touch_position_t max = { 0, 0, 0 };

void touch_position_print(touch_position_t &v) {
    Serial.print(v.x); Serial.print(", ");
    Serial.print(v.y); Serial.print(", ");
    Serial.print(v.z);
}

static bool touched = false;

void isr_touched() {
    touched = true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Adafruit STMPE610 example");
  Serial.flush();

  pinMode(5, INPUT);
  attachInterrupt(digitalPinToInterrupt(5), isr_touched, CHANGE);

  if (!touch.begin()) {
    Serial.println("STMPE not found!");
    while(1);
  }
}

void loop() {
  if (touched) {
      Serial.println("Touched!");
      touched = false;
  }
  if (touch.touched()) {
    while (!touch.bufferEmpty()) {
      touch_position_t position;
      Serial.print(touch.bufferSize());

      touch.readData(&position.x, &position.y, &position.z);

      if (min.x > position.x) min.x = position.x;
      if (min.y > position.y) min.y = position.y;
      if (min.z > position.z) min.z = position.z;

      if (max.x < position.x) max.x = position.x;
      if (max.y < position.y) max.y = position.y;
      if (max.z < position.z) max.z = position.z;

      Serial.print(" min(");
      touch_position_print(min);
      Serial.print(")");

      Serial.print(" max(");
      touch_position_print(max);
      Serial.print(")");

      Serial.print("->(");
      touch_position_print(position);
      Serial.print(")");

      Serial.println("");
    }
    touch.writeRegister8(STMPE_INT_STA, 0xFF);
  }
  delay(10);
}
