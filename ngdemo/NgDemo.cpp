#include "NgDemo.h"

NgDemo::NgDemo()
    : gps(&Serial2) {
}

bool NgDemo::setup() {
    platformSerial2Begin(9600);

    gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
    gps.sendCommand(PGCMD_ANTENNA);

    state = NgDemoState::WaitingGpsFix;
    stateChangedAt = millis();

    return true;
}

bool NgDemo::preflight() {
    digitalWrite(PIN_RED_LED, HIGH);

    // Check GPS
    // Check RockBlock
    // Check Sensor

    digitalWrite(PIN_RED_LED, LOW);

    return true;
}

void NgDemo::tick() {
    switch (state) {
    case NgDemoState::WaitingGpsFix: {
        Watchdog.reset();
        if (checkGps()) {
            state = NgDemoState::ReadingSensors;
        }
        else if (millis() - stateChangedAt > STATE_MAX_GPS_FIX_TIME) {
            state = NgDemoState::ReadingSensors;
        }
        break;
    }
    case NgDemoState::ReadingSensors: {
        Serial.println();

        DHT dht(PIN_DHT, DHT22);
        dht.begin();
        humidity = dht.readHumidity();
        temperature = dht.readTemperature();
        batteryLevel = platformBatteryLevel();

        Serial.println(humidity);
        Serial.println(temperature);
        Serial.println(batteryLevel);

        state = NgDemoState::Transmitting;
        stateChangedAt = millis();
        break;
    }
    case NgDemoState::Transmitting: {
        uint32_t uptime = millis() / (1000 * 60);
        String message(SystemClock->now());
        message += ",";
        message += "NGD";
        message += ",";
        message += latitude;
        message += ",";
        message += longitude;
        message += ",";
        message += altitude;
        message += ",";
        message += temperature;
        message += ",";
        message += humidity;
        message += ",";
        message += batteryLevel;
        message += ",";
        message += uptime;
        transmission(message);

        state = NgDemoState::Sleep;
        stateChangedAt = millis();
        break;
    }
    case NgDemoState::Sleep: {
        Watchdog.reset();
        if (millis() - stateChangedAt > STATE_MAX_SLEEP_TIME) {
            state = NgDemoState::WaitingGpsFix;
            stateChangedAt = millis();
        }
        break;
    }
    }
}

bool NgDemo::transmission(String message) {
    Serial.print("Message: ");
    Serial.println(message.c_str());

    digitalWrite(PIN_RED_LED, HIGH);

    bool success = false;
    uint32_t started = millis();
    if (message.length() > 0) {
        RockBlock rockBlock(message);
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
        Serial.print("RockBlock: ");
        Serial.println(success);
    }

    digitalWrite(PIN_RED_LED, LOW);

    return success;
}

bool NgDemo::checkGps() {
    Watchdog.reset();

    if (Serial2.available()) {
        while (Serial2.available()) {
            char c = gps.read();
            Serial.print(c);
        }
    }
    if (gps.newNMEAreceived()) {
        if (gps.parse(gps.lastNMEA())) {
            if (gps.fix) {
                Serial.println("GOT NMEA");
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
