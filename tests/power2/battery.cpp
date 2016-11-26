#include "battery.h"

float battery_voltage_get(uint8_t pin) {
  pinMode(pin, INPUT);

  analogRead(pin);
  analogRead(pin);
  float value = analogRead(pin);
  return value * BATTERY_VOLTAGE_DIVIDER_RATIO * BATTERY_VOLTAGE_REFERENCE / 1024.0f;
}

float battery_voltage_to_level(float voltage) {
  float constrained = max(min(voltage, BATTERY_VOLTAGE_OPTIMAL), BATTERY_VOLTAGE_LOW);
  return (constrained - BATTERY_VOLTAGE_LOW) / (BATTERY_VOLTAGE_OPTIMAL - BATTERY_VOLTAGE_LOW);
}
