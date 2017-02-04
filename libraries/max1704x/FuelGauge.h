#ifndef FUEL_GAUGE_H_INCLUDED
#define FUEL_GAUGE_H_INCLUDED

#include <Arduino.h>
#include <Wire.h>

class FuelGauge {
private:
    uint8_t address;

public:
    FuelGauge();

public:
    void powerOn();
    void version();
    void config();
    float stateOfCharge();
    float cellVoltage();

};

#endif
