#ifndef TRANSMISSION_STATUS_H
#define TRANSMISSION_STATUS_H

#define TRANSMISSION_KIND_LOCATION  0
#define TRANSMISSION_KIND_SENSORS   1
#define TRANSMISSION_KIND_WEATHER   2
#define TRANSMISSION_KIND_KINDS     3

typedef struct fk_transmission_schedule_t {
    uint8_t offset;
    uint8_t interval;
} fk_transmission_schedule_t;

class TransmissionStatus {
public:
    bool anyTransmissionsThisHour();
    int8_t shouldWe(fk_transmission_schedule_t *schedules);

};

#endif
