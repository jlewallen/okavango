#include <SPI.h>
#include <Adafruit_SleepyDog.h>
#include "WifiConnection.h"

#define PIN_WINC_CS   8
#define PIN_WINC_IRQ  7
#define PIN_WINC_RST  4
#define PIN_WINC_EN   2

Adafruit_WINC1500 WiFi(PIN_WINC_CS, PIN_WINC_IRQ, PIN_WINC_RST);
Adafruit_WINC1500Client client;

void logNetworkInformation(Stream &stream) {
    IPAddress ip = WiFi.localIP();
    stream.print("IP Address: ");
    stream.println(ip);

    byte mac[6];
    WiFi.macAddress(mac);
    stream.print("MAC address: ");
    stream.print(mac[5], HEX);
    stream.print(":");
    stream.print(mac[4], HEX);
    stream.print(":");
    stream.print(mac[3], HEX);
    stream.print(":");
    stream.print(mac[2], HEX);
    stream.print(":");
    stream.print(mac[1], HEX);
    stream.print(":");
    stream.println(mac[0], HEX);
}

WifiConnection::WifiConnection(const char *ssid, const char *psk, Stream &logStream) :
    ssid(ssid), psk(psk), status(WL_IDLE_STATUS), logStream(logStream) {
}

void WifiConnection::on() {
    pinMode(PIN_WINC_EN, OUTPUT);
    digitalWrite(PIN_WINC_EN, HIGH);

}

void WifiConnection::off() {
    digitalWrite(PIN_WINC_EN, LOW);
}

bool WifiConnection::open() {
    on();

    if (WiFi.status() == WL_NO_SHIELD) {
        logStream.println("Wifi: missing");
        DEBUG_PRINTLN("Wifi: missing");
        return false;
    }

    uint32_t started = millis();
    while (WiFi.status() != WL_CONNECTED) {
        logStream.print("Wifi: attempting ");
        logStream.println(ssid);
        DEBUG_PRINT("Wifi: attempting ");
        DEBUG_PRINTLN(ssid);

        status = WiFi.begin(ssid, psk);

        uint8_t seconds = 10;
        while (seconds > 0 && (WiFi.status() != WL_CONNECTED)) {
            seconds--;
            delay(1000);
            Watchdog.reset();
        }

        if (millis() - started > ONE_MINUTE) {
            break;
        }
    }

    logNetworkInformation(Serial);
    logNetworkInformation(logStream);

    status = WiFi.status();

    if (status != WL_CONNECTED) {
        off();
        return false;
    }

    return true;
}

bool WifiConnection::post(const char *server, const char *path, const char *contentType, const char *body) {
    Watchdog.disable();

    bool success = false;

    if (client.connect(server, 80)) {
        DEBUG_PRINTLN("Connected");
        logStream.println("Connected, sending:");

        logStream.println(body);

        client.print("POST ");
        client.print(path);
        client.println(" HTTP/1.1");
        client.print("Host: "); client.println(server);
        client.print("Content-Type: "); client.println(contentType);
        client.print("Content-Length: "); client.println(strlen(body));
        client.println("Connection: close");
        client.println();
        client.println(body);

        uint32_t started = millis();
        while (millis() - started < ONE_MINUTE) {
            delay(10);

            Watchdog.reset();

            while (client.available()) {
                Serial.write(client.read());
            }

            if (!client.connected()) {
                DEBUG_PRINTLN();
                DEBUG_PRINTLN("Yay");
                logStream.println("Closed");
                client.stop();
                success = true;
                break;
            }
        }
    }
    else {
        logStream.println("Unable to connect.");
    }

    Watchdog.enable();
    return success;
}
