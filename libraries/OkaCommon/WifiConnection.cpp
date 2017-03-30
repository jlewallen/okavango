#include <SPI.h>
#include <Adafruit_SleepyDog.h>
#include "WifiConnection.h"
#include <driver/source/nmasic.h>

WiFiClient client;

void firmwareCheck() {
    String fv = WiFi.firmwareVersion();
    String latestFv;
    Serial.print("Firmware version: ");
    Serial.println(fv);

    if (REV(GET_CHIPID()) >= REV_3A0) {
        latestFv = WIFI_FIRMWARE_LATEST_MODEL_B;
    } else {
        latestFv = WIFI_FIRMWARE_LATEST_MODEL_A;
    }

    Serial.print("Latest available: ");
    Serial.println(latestFv);

    if (fv == latestFv) {
        Serial.println("Firmware check: PASSED");
    }
    else {
        Serial.println("Firmware check: FAILED");
    }
}

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
    off();

    delay(500);

    on();

    delay(500);

    if (WiFi.status() == WL_NO_SHIELD) {
        DEBUG_PRINTLN("Wifi: missing");
        return false;
    }

    firmwareCheck();

    uint32_t started = millis();
    while (WiFi.status() != WL_CONNECTED) {
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

    status = WiFi.status();

    if (status != WL_CONNECTED) {
        WiFi.end();
        off();
        return false;
    }

    return true;
}

bool WifiConnection::post(const char *server, const char *path, const char *contentType, const char *body) {
    DEBUG_PRINTLN(body);

    return post(server, path, contentType, (uint8_t *)body, strlen(body));
}

bool WifiConnection::post(const char *server, const char *path, const char *contentType, const uint8_t *body, size_t length) {
    Watchdog.disable();

    bool success = false;

    client.stop();

    if (client.connect(server, 80)) {
        DEBUG_PRINTLN("Connected");

        client.print("POST ");
        client.print(path);
        client.println(" HTTP/1.1");
        client.print("Host: "); client.println(server);
        client.print("Content-Type: "); client.println(contentType);
        client.print("Content-Length: "); client.println(length);
        client.println("Connection: close");
        client.println();
        client.write(body, length);

        uint32_t started = millis();
        while (millis() - started < ONE_MINUTE) {
            delay(10);

            Watchdog.reset();

            while (client.available()) {
                Serial.write(client.read());
            }

            if (!client.connected()) {
                client.stop();
                success = true;
                break;
            }
        }
    }
    else {
        DEBUG_PRINTLN("Unable to connect.");
    }

    client.stop();

    Watchdog.enable();
    return success;
}
