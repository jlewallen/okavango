#ifndef DS18B20_H
#define DS18B20_H

#include <OneWire.h>
#include <DallasTemperature.h>

class Ds18B20 {
private:
    OneWire oneWire;
    DallasTemperature dallasTemperature;
    bool available;

public:
    Ds18B20(uint8_t pin);

public:
    bool setup();
    float getTemperature();
};

#endif
