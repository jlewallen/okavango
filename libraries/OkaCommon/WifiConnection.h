#ifndef WIFI_CONNECTION_H
#define WIFI_CONNECTION_H

#include "Platforms.h"
#include <WiFi101.h>

#define PIN_WINC_CS   8
#define PIN_WINC_IRQ  7
#define PIN_WINC_RST  4
#define PIN_WINC_EN   2

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
