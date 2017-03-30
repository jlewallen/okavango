#ifndef WIFI_CONNECTION_H
#define WIFI_CONNECTION_H

#include "Platforms.h"
#include <Adafruit_WINC1500.h>

class WifiConnection {
private:
    const char *ssid;
    const char *psk;
    int32_t status;
    Stream &logStream;

public:
    WifiConnection(const char *ssid, const char *psk, Stream &logStream);

public:
    void off();
    void on();
    bool open();
    bool post(const char *server, const char *path, const char *contentType, const char *body);
    bool post(const char *server, const char *path, const char *contentType, const uint8_t *body, size_t length);
};

#endif
