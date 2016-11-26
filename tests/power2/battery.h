#ifndef BATTERY_H_INCLUDED
#define BATTERY_H_INCLUDED

#include <Arduino.h>

#define BATTERY_VOLTAGE_DIVIDER_RATIO                         2.0f
#define BATTERY_VOLTAGE_REFERENCE                             3.3f
#define BATTERY_VOLTAGE_OPTIMAL                               4.2f
#define BATTERY_VOLTAGE_LOW                                   3.0f

float battery_voltage_get(uint8_t pin);

float battery_voltage_to_level(float voltage);

#endif
