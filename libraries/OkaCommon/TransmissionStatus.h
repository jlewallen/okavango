#ifndef TRANSMISSION_STATUS_H
#define TRANSMISSION_STATUS_H

#define TRANSMISSION_KIND_LOCATION  0
#define TRANSMISSION_KIND_SENSORS   1
#define TRANSMISSION_KIND_WEATHER   2
#define TRANSMISSION_KIND_KINDS     3

class TransmissionStatus {
public:
    void startup();
    bool anyTransmissionsThisHour();
    int8_t shouldWe();
};

#endif
