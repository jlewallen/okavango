#include "WeatherStation.h"
#include "protocol.h"
#include "Logger.h"
#include "Diagnostics.h"

#ifdef ARDUINO_SAMD_FEATHER_M0

#define WEATHER_STATION_INTERVAL_START                        (1000 * 60)
#define WEATHER_STATION_INTERVAL_IGNORE                       (1000 * 60 * 5)
#define WEATHER_STATION_INTERVAL_OFF                          (1000 * 60 * 5)
#define WEATHER_STATION_INTERVAL_READING                      (1000 * 60 * 2)

WeatherStation::WeatherStation() {
    clear();
    memzero((uint8_t *)&fix, sizeof(gps_fix_t));
}

void WeatherStation::setup() {
    off();
    transition(WeatherStationState::Ignoring);
}

void WeatherStation::clear() {
    numberOfValues = 0;
    buffer[0] = 0;
    length = 0;
    hasUnreadReading = false;
}

void WeatherStation::ignore() {
    while (WeatherSerial.available()) {
        WeatherSerial.read();
    }
}

void WeatherStation::off() {
    DEBUG_PRINTLN("WS: off");

    WeatherSerial.end();

    pinMode(PIN_WEATHER_STATION_RESET, OUTPUT);
    digitalWrite(PIN_WEATHER_STATION_RESET, LOW);
    on = false;
}

void WeatherStation::hup() {
    DEBUG_PRINTLN("WS: hup");

    weatherSerialBegin();

    pinMode(PIN_WEATHER_STATION_RESET, OUTPUT);
    digitalWrite(PIN_WEATHER_STATION_RESET, LOW);
    delay(500);
    digitalWrite(PIN_WEATHER_STATION_RESET, HIGH);
    delay(500);
    on = true;
    checkingCommunications = false;
}

bool WeatherStation::tick() {
    switch (state) {
    case WeatherStationState::Start: {
        if (on) {
            off();
        }
        if (millis() - lastTransitionAt > WEATHER_STATION_INTERVAL_START) {
            DEBUG_PRINTLN("WS: >Ignoring");
            transition(WeatherStationState::Ignoring);
        }
        break;
    }
    case WeatherStationState::Ignoring: {
        if (!on) {
            hup();
        }
        ignore();
        if (millis() - lastTransitionAt > WEATHER_STATION_INTERVAL_IGNORE) {
            DEBUG_PRINTLN("WS: >Reading");
            transition(WeatherStationState::Reading);
            startReading = false;
        }
        break;
    }
    case WeatherStationState::Reading: {
        if (millis() - lastTransitionAt > WEATHER_STATION_INTERVAL_READING) {
            DEBUG_PRINTLN("WS: >Ignoring");
            transition(WeatherStationState::Ignoring);
            break;
        }
        if (WeatherSerial.available()) {
            delay(50);

            while (WeatherSerial.available()) {
                int16_t c = WeatherSerial.read();
                if (c >= 0) {
                    if (!startReading) {
                        if (c == '\n') {
                            startReading = true;
                        }
                    }
                    else if (c == ',' || c == '\r' || c == '\n') {
                        if (length > 0) {
                            buffer[length] = 0;
                            if (numberOfValues < FK_WEATHER_STATION_MAX_VALUES) {
                                values[numberOfValues] = atof(buffer);
                                // DEBUG_PRINT("Parsed ");
                                // DEBUG_PRINT(buffer);
                                // DEBUG_PRINT(" = ");
                                // DEBUG_PRINTLN(values[numberOfValues]);
                                numberOfValues++;
                            }
                            length = 0;;
                        }
                        if (c == '\r' || c == '\n') {
                            bool success = numberOfValues == FK_WEATHER_STATION_PACKET_NUMBER_VALUES;
                            if (success) {
                                DEBUG_PRINTLN("WS: have reading");
                                for (uint8_t i = 0; i < FK_WEATHER_STATION_PACKET_NUMBER_VALUES; ++i) {
                                    if (i > 0) {
                                        DEBUG_PRINT(",");
                                    }
                                    DEBUG_PRINT(values[i]);
                                }
                                DEBUG_PRINTLN("");

                                // Sanity check the reading.
                                #define JANUARY_1ST_2020   (1577836800)
                                #define AUGUST_29TH_2016   (1472428800)

                                bool hasGoodFix =
                                    (values[FK_WEATHER_STATION_FIELD_UNIXTIME] < JANUARY_1ST_2020 && values[FK_WEATHER_STATION_FIELD_UNIXTIME] > AUGUST_29TH_2016) &&
                                    (values[FK_WEATHER_STATION_FIELD_SATELLITES] > 0);

                                if (hasGoodFix) {
                                    fix.time = values[FK_WEATHER_STATION_FIELD_UNIXTIME];
                                    fix.latitude = values[FK_WEATHER_STATION_FIELD_LATITUDE];
                                    fix.longitude = values[FK_WEATHER_STATION_FIELD_LONGITUDE];
                                    fix.altitude = values[FK_WEATHER_STATION_FIELD_ALTITUDE];
                                    fix.satellites = values[FK_WEATHER_STATION_FIELD_SATELLITES];
                                    fix.valid = true;
                                }
                                else {
                                    fix.time = 0;
                                    fix.latitude = 0;
                                    fix.longitude = 0;
                                    fix.altitude = 0;
                                    fix.satellites = 0;
                                    fix.valid = false;
                                }

                                hasUnreadReading = true;

                                diagnostics.updateGpsStatus(hasGoodFix);
                                diagnostics.recordWeatherReading();

                                if (checkingCommunications) {
                                    DEBUG_PRINTLN("WS: >CommunicationsOk");
                                    transition(WeatherStationState::CommunicationsOk);
                                }
                                else {
                                    DEBUG_PRINTLN("WS: >Off");
                                    transition(WeatherStationState::Off);
                                }
                            }
                            else {
                                DEBUG_PRINT("WS: no reading: ");
                                DEBUG_PRINT(numberOfValues);
                                DEBUG_PRINT(" ");
                                DEBUG_PRINT(FK_WEATHER_STATION_PACKET_NUMBER_VALUES);
                                DEBUG_PRINTLN("");
                            }

                            numberOfValues = 0;

                            break;
                        }
                    }
                    else if (length < FK_WEATHER_STATION_MAX_BUFFER - 1) {
                        buffer[length++] = (char)c;
                        buffer[length] = 0;
                    }
                }
            }
        }
        break;
    }
    case WeatherStationState::Off: {
        if (WEATHER_STATION_INTERVAL_OFF > 0) {
            if (on) {
                off();
            }
        }
        if (WEATHER_STATION_INTERVAL_OFF == 0 || millis() - lastTransitionAt > WEATHER_STATION_INTERVAL_OFF) {
            DEBUG_PRINTLN("WS: >Ignoring");
            transition(WeatherStationState::Ignoring);
        }
        break;
    }
    }

    return false;
}

void WeatherStation::logReadingLocally() {
    File file = Logger::open(FK_SETTINGS_WEATHER_STATION_DATA_FILENAME);
    if (file) {
        Serial.print("*");
        for (uint8_t i = 0; i < FK_WEATHER_STATION_PACKET_NUMBER_VALUES; ++i) {
            if (i > 0) {
                file.print(",");
            }
            file.print(values[i]);
        }
        file.println();
        file.close();
    }
    else {
        Serial.println("Unable to open WeatherStation log");
    }
}

#endif
