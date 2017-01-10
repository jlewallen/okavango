#ifndef INITIAL_TRANSMISSIONS_H_INCLUDED
#define INITIAL_TRANSMISSIONS_H_INCLUDED

#include <Arduino.h>

#define TRANSMISSION_TYPE_WEATHER     0x1
#define TRANSMISSION_TYPE_ATLAS       0x2
#define TRANSMISSION_TYPE_SONAR       0x3
#define TRANSMISSION_TYPE_LOCATION    0x4

class InitialTransmissions {
public:
    static void markCompleted(uint8_t which);
    static bool alreadyDone(uint8_t which);
};

extern bool initialWeatherTransmissionSent;
extern bool initialAtlasTransmissionSent;
extern bool initialSonarTransmissionSent;
extern bool initialLocationTransmissionSent;

#endif
