#include "Ds18B20.h"

Ds18B20::Ds18B20(uint8_t pin) : oneWire(pin),  dallasTemperature(&oneWire) {
}

bool Ds18B20::setup() {
    dallasTemperature.begin();
    available = dallasTemperature.getDeviceCount() > 0;
    return available;
}

float Ds18B20::getTemperature() {
    dallasTemperature.requestTemperatures();
    return dallasTemperature.getTempCByIndex(0);
}
