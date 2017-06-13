#include <IridiumSBD.h>
#include <SD.h>
#include "NgDemo.h"
#include "Diagnostics.h"
#include "WifiConnection.h"
#include "WatchdogCallbacks.h"
#include "Logger.h"

#define PIN_DHT                                           18
#define PIN_OUTSIDE_LED1                                  12
#define PIN_OUTSIDE_LED2                                  11
#define PIN_OUTSIDE_LED3                                  10

NgDemo::NgDemo() :
    gps(&Serial1), data("DATA.BIN") {
}

bool NgDemo::setup() {
    platformSerial2Begin(9600);
    Serial1.begin(9600);

    if (!config.read()) {
        platformCatastrophe(PIN_RED_LED);
    }

    state = NgDemoState::WaitingGpsFix;
    stateChangedAt = millis();

    pinMode(PIN_OUTSIDE_LED1, OUTPUT);
    digitalWrite(PIN_OUTSIDE_LED1, LOW);
    pinMode(PIN_OUTSIDE_LED2, OUTPUT);
    digitalWrite(PIN_OUTSIDE_LED2, LOW);
    pinMode(PIN_OUTSIDE_LED3, OUTPUT);
    digitalWrite(PIN_OUTSIDE_LED3, LOW);

    return true;
}

void NgDemo::failPreflight(uint8_t kind) {
    Watchdog.reset();

    logPrinter.flush();

    digitalWrite(PIN_RED_LED, LOW);

    while (true) {
        for (uint8_t i = 0; i < kind; ++i) {
            digitalWrite(PIN_RED_LED, HIGH);
            delay(250);
            digitalWrite(PIN_RED_LED, LOW);
            delay(250);
        }
        delay(1000);
    }
}

bool NgDemo::configure() {
    gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
    gps.sendCommand(PGCMD_ANTENNA);

    return true;
}

bool NgDemo::preflight() {
    DEBUG_PRINTLN("preflight: Start");

    #ifdef NGD_ROCKBLOCK
    // Check RockBlock
    rockBlockSerialBegin();
    IridiumSBD rockBlock(Serial2, PIN_ROCK_BLOCK, new WatchdogCallbacks());
    if (Serial) {
        rockBlock.attachConsole(Serial);
        rockBlock.attachDiags(Serial);
    }
    else {
        rockBlock.attachConsole(logPrinter);
        rockBlock.attachDiags(logPrinter);
    }
    rockBlock.setPowerProfile(1);
    if (rockBlock.begin(10) != ISBD_SUCCESS) {
        DEBUG_PRINTLN("preflight: RockBlock failed");
        failPreflight(1);
    }

    DEBUG_PRINTLN("preflight: RockBlock good");
    #endif

    #ifdef NGD_WIFI
    WiFi.setPins(PIN_WINC_CS, PIN_WINC_IRQ, PIN_WINC_RST, PIN_WINC_EN);

    WifiConnection wifi(config.getSsid(), config.getPassword(), Serial);
    if (!wifi.open()) {
        DEBUG_PRINTLN("preflight: WiFi failed");
        failPreflight(1);
    }

    wifi.off();
    DEBUG_PRINTLN("preflight: WiFi good");
    #endif

    // Check Sensor
    DHT dht(PIN_DHT, DHT22);
    dht.begin();
    if (dht.readHumidity() == NAN || dht.readTemperature() == NAN) {
        DEBUG_PRINTLN("preflight: Sensors failed");
        failPreflight(2);
    }

    DEBUG_PRINTLN("preflight: DHT22 good");

    // Check GPS
    uint32_t startTime = millis();
    uint32_t queryTime = millis();

    while (true) {
        delay(10);

        while (Serial1.available()) {
            char c = gps.read();
            Serial.print(c);
        }

        if (millis() - startTime > STATE_MAX_PREFLIGHT_GPS_TIME) {
            DEBUG_PRINTLN("");
            DEBUG_PRINTLN("preflight: GPS failed");
            failPreflight(3);
        }
        else if (millis() - queryTime > 2000) {
            gps.sendCommand(PMTK_Q_RELEASE);
        }

        if (gps.newNMEAreceived()) {
            const char *nmea = gps.lastNMEA();
            if (strstr(nmea, "$PMTK705") >= 0) {
                break;
            }
        }

        Watchdog.reset();
    }

    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("preflight: GPS good");

    DEBUG_PRINTLN("preflight: Passed");

    return true;
}

void NgDemo::tick() {
    switch (state) {
    case NgDemoState::WaitingGpsFix: {
        Watchdog.reset();
        if (checkGps()) {
            DEBUG_PRINTLN("GPS: Fix");
            state = NgDemoState::ReadingSensors;
            logPrinter.flush();
        }
        else if (millis() - stateChangedAt > STATE_MAX_GPS_FIX_TIME) {
            DEBUG_PRINTLN("GPS: No Fix");
            state = NgDemoState::ReadingSensors;
            logPrinter.flush();
        }
        break;
    }
    case NgDemoState::ReadingSensors: {
        DEBUG_PRINT("ReadingSensors: ");

        DHT dht(PIN_DHT, DHT22);
        dht.begin();
        humidity = dht.readHumidity();
        temperature = dht.readTemperature();
        batteryLevel = platformBatteryLevel();
        batteryVoltage = platformBatteryVoltage();

        DEBUG_PRINT(humidity);
        DEBUG_PRINT(", ");
        DEBUG_PRINT(temperature);
        DEBUG_PRINTLN("");

        uint8_t buffer[128];
        DEBUG_PRINTLN("Encoding and queueing message...");
        messageSize = encodeMessage(buffer, sizeof(buffer));

        DEBUG_PRINT("Message: ");
        DEBUG_PRINTLN(messageSize);
        data.enqueue(buffer, sizeof(buffer));

        File file = Logger::open("DATA.CSV");
        if (file) {
            float uptime = millis() / (1000 * 60);
            float values[8] = {
                latitude,
                longitude,
                altitude,
                temperature,
                humidity,
                batteryLevel,
                batteryVoltage,
                uptime,
            };
            file.print(SystemClock->now());
            for (uint8_t i = 0; i < 8; ++i) {
                file.print(",");
                file.print(values[i]);
            }
            file.println();
            file.close();
        }
        else {
            Serial.println("Unable to open log");
        }

        state = NgDemoState::Transmitting;
        stateChangedAt = millis();
        logPrinter.flush();
        break;
    }
    case NgDemoState::Transmitting: {
        digitalWrite(PIN_RED_LED, HIGH);
        transmission();
        digitalWrite(PIN_RED_LED, LOW);

        state = NgDemoState::Sleep;
        stateChangedAt = millis();
        batteryLoggedAt = millis();
        logPrinter.flush();
        break;
    }
    case NgDemoState::Sleep: {
        Watchdog.reset();
        if (millis() - stateChangedAt > STATE_MAX_SLEEP_TIME) {
            DEBUG_PRINTLN("Waking up...");
            state = NgDemoState::WaitingGpsFix;
            stateChangedAt = millis();
            logPrinter.flush();
        }

        if (millis() - batteryLoggedAt > 10000) {
            batteryLevel = platformBatteryLevel();
            batteryVoltage = platformBatteryVoltage();
            DEBUG_PRINT(batteryVoltage);
            DEBUG_PRINT(" ");
            DEBUG_PRINT(batteryLevel);
            DEBUG_PRINTLN();
            logPrinter.flush();
            batteryLoggedAt = millis();
        }
        break;
    }
    }
}

template<typename int_t = uint64_t>
size_t encode_varint(int_t value, uint8_t *buffer) {
    size_t written = 0;
    while (value > 127) {
        buffer[written] = ((uint8_t)(value & 127)) | 128;
        value >>= 7;
        written++;
    }
    buffer[written++] = ((uint8_t)value) & 127;
    return written;
}

size_t NgDemo::encodeMessage(uint8_t *buffer, size_t bufferSize) {
    float uptime = millis() / (1000 * 60);
    float values[8] = {
        latitude,
        longitude,
        altitude,
        temperature,
        humidity,
        batteryLevel,
        batteryVoltage,
        uptime
    };

    uint8_t *ptr = buffer;
    ptr += encode_varint(1, ptr);
    ptr += encode_varint(SystemClock->now(), ptr);
    memcpy(ptr, values, sizeof(values));
    ptr += sizeof(values);

    return ptr - buffer;
}

bool NgDemo::transmission() {
    bool success = false;

    #ifdef NGD_WIFI
    WifiConnection wifi(config.getSsid(), config.getPassword(), Serial);
    if (wifi.open()) {
        FileQueue failed("FAILED.BIN");

        data.startAtBeginning();

        while (true) {
            uint8_t *message = (uint8_t *)data.dequeue();
            if (message == NULL) {
                break;
            }

            DEBUG_PRINTLN("Dequeued, attempting to send...");

            if (!wifi.post(config.getUrlServer(), config.getUrlPath(), "application/octet-stream", message, messageSize)) {
                failed.enqueue((uint8_t *)message);
            }
        }

        failed.copyInto(&data);
        failed.removeAll();

        WiFi.disconnect();
        WiFi.end();

        wifi.off();
    }
    #endif

    #ifdef NGD_ROCKBLOCK
    uint8_t buffer[128];

    DEBUG_PRINTLN("Encoding message...");

    size_t size = encodeMessage(buffer, sizeof(buffer));
    if (size == 0) {
        return false;
    }
    else {
        DEBUG_PRINT("Message: ");
        DEBUG_PRINTLN(size);
    }

    logPrinter.flush();

    uint32_t started = millis();
    if (size > 0) {
        for (uint8_t i = 0; i < 2; ++i) {
            #if 0
            RockBlock rockBlock(buffer, size);
            rockBlockSerialBegin();
            SerialType &rockBlockSerial = RockBlockSerial;
            rockBlock.setSerial(&rockBlockSerial);
            while (!rockBlock.isDone() && !rockBlock.isFailed()) {
                if (millis() - started < THIRTY_MINUTES) {
                    Watchdog.reset();
                }
                rockBlock.tick();
                delay(10);
            }
            success = rockBlock.isDone();
            #endif
            DEBUG_PRINT("RockBlock: ");
            DEBUG_PRINTLN(success);
            if (success) {
                break;
            }
        }
    }
    #endif

    logPrinter.flush();

    return success;
}

bool NgDemo::checkGps() {
    Watchdog.reset();

    while (Serial1.available()) {
        char c = gps.read();
        Serial.print(c);
    }

    if (gps.newNMEAreceived()) {
        if (gps.parse(gps.lastNMEA())) {
            if (gps.fix) {
                DateTime dateTime = DateTime(gps.year, gps.month, gps.year, gps.hour, gps.minute, gps.seconds);
                uint32_t time = dateTime.unixtime();
                SystemClock->set(time);
                latitude = gps.latitudeDegrees;
                longitude = gps.longitudeDegrees;
                altitude = gps.altitude;
                return true;
            }
        }
    }
    return false;
}
