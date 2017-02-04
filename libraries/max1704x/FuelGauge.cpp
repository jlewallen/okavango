#include <Wire.h>
#include "FuelGauge.h"

FuelGauge::FuelGauge() :
    address(0x36) {
}

void FuelGauge::powerOn() {
    Wire.beginTransmission(address);
    Wire.write((byte)0xfe);
    Wire.write((byte)0x54);
    Wire.write((byte)0x00);
    Wire.endTransmission();
}

void FuelGauge::version() {
    Wire.beginTransmission(address);
    Wire.write((byte)0x08);
    Wire.endTransmission();

    Wire.requestFrom(address, (byte)2);
    uint8_t high = Wire.read();
    uint8_t low = Wire.read();

    Serial.println(high);
    Serial.println(low);
}

void FuelGauge::config() {
    Wire.beginTransmission(address);
    Wire.write((byte)0x0C);
    Wire.endTransmission();

    Wire.requestFrom(address, (byte)2);
    uint8_t high = Wire.read();
    uint8_t low = Wire.read();

    Serial.println(high);
    Serial.println(low);
}

float FuelGauge::cellVoltage() {
    Wire.beginTransmission(address);
    Wire.write((byte)0x02);
    Wire.endTransmission();

    Wire.requestFrom(address, (byte)2);
    uint8_t high = Wire.read();
    uint8_t low = Wire.read();
    uint32_t temp = ((low | (high << 8)) >> 4);

    // 1.25mv for version 3 and 2.50mv for version 4;
    return 1.25 * temp;
}

float FuelGauge::stateOfCharge() {
    Wire.beginTransmission(address);
    Wire.write((byte)0x04);
    Wire.endTransmission();

    Wire.requestFrom(address, (byte)2);
    uint8_t high = Wire.read();
    uint8_t low = Wire.read();

    return high + ((float)low / 256.0f);
}
