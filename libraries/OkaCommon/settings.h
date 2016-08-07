#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

#define FK_SETTINGS_NAME_LENGTH                     2

typedef struct fk_settings_t {
    char name[FK_SETTINGS_NAME_LENGTH];
} fk_settings_t;

#endif
