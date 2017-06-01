#ifndef SERIALS_H
#define SERIALS_H

#include <Arduino.h>

#ifdef ARDUINO_AVR_FEATHER32U4
#include <SoftwareSerial.h>

typedef Stream SerialType;

extern SoftwareSerial Serial2;

extern void platformSerial2Begin(int32_t baud);
#endif

#ifdef ARDUINO_SAMD_FEATHER_M0
extern Uart Serial2;

typedef Uart SerialType;

extern void platformSerial2Begin(int32_t baud);
#endif

#endif
