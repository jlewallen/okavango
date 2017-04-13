#include <Adafruit_SleepyDog.h>
#include "WeatherStation.h"
#include "protocol.h"
#include "Logger.h"
#include "Diagnostics.h"
#include "Queue.h"

#define WEATHER_STATION_READ_TIME         (20 * 1000)

#ifdef ARDUINO_SAMD_FEATHER_M0

WeatherStation::WeatherStation(Memory *memory, FuelGauge *gauge) :
    memory(memory), gauge(gauge) {
    clear();
    memzero((uint8_t *)&fix, sizeof(gps_fix_t));
}

void WeatherStation::setup() {
    off();
    transition(WeatherStationState::Start);
}

void WeatherStation::clear() {
    numberOfValues = 0;
    buffer[0] = 0;
    length = 0;
    hasUnreadReading = false;
}

void WeatherStation::off() {
    DEBUG_PRINTLN("WS: off");

    WeatherSerial.end();

    pinMode(PIN_WEATHER_STATION_RESET, OUTPUT);
    digitalWrite(PIN_WEATHER_STATION_RESET, LOW);
    on = false;
    clockSet = false;
}

void WeatherStation::hup() {
    if (!on) {
        DEBUG_PRINTLN("WS: on");
    }

    pinMode(PIN_WEATHER_STATION_RESET, OUTPUT);
    digitalWrite(PIN_WEATHER_STATION_RESET, LOW);
    delay(500);
    digitalWrite(PIN_WEATHER_STATION_RESET, HIGH);
    delay(500);
    on = true;
    checkingCommunications = false;
}

bool WeatherStation::tick() {
    fk_memory_weather_intervals_t *intervals = &memory->intervals()->weatherStation;

    DateTime dt(SystemClock->now());

    if (state != WeatherStationState::CommunicationsOk) {
        bool shouldBeOn = checkingCommunications || dt.minute() < intervals->stop;
        if (on && !shouldBeOn) {
            DEBUG_PRINTLN("WS: >Off");
            transition(WeatherStationState::Off);
        }
        else if (!on && shouldBeOn) {
            DEBUG_PRINT("WS: >Waiting ");
            DEBUG_PRINT(checkingCommunications);
            DEBUG_PRINT(", ");
            DEBUG_PRINT(dt.minute());
            DEBUG_PRINT(" < ");
            DEBUG_PRINTLN(intervals->stop);
            transition(WeatherStationState::Waiting);
        }
    }

    switch (state) {
    case WeatherStationState::Start: {
        break;
    }
    case WeatherStationState::Waiting: {
        if (!on) {
            hup();
        }
        if (platformUptime() - lastTransitionAt > 60 * 1000) {
            DEBUG_PRINTLN("WS: >Reading");
            transition(WeatherStationState::Reading);
        }
        break;
    }
    case WeatherStationState::Off: {
        if (on) {
            off();
        }
        break;
    }
    case WeatherStationState::Reading: {
        Watchdog.enable();

        startReading = false;

        WeatherSerial.end();

        delay(100);

        weatherSerialBegin();

        delay(100);

        uint32_t started = platformUptime();
        uint16_t bytesRead = 0;

        while (state == WeatherStationState::Reading) {
            delay(10);

            Watchdog.reset();

            if (platformUptime() - lastTransitionAt > WEATHER_STATION_READ_TIME) {
                DEBUG_PRINT("WS: >Waiting (no reading) ms=");
                DEBUG_PRINT(platformUptime() - started);
                DEBUG_PRINT(" ");
                DEBUG_PRINTLN(bytesRead);
                transition(WeatherStationState::Waiting);
            }
            else if (WeatherSerial.available()) {
                while (WeatherSerial.available()) {
                    int16_t c = WeatherSerial.read();
                    bytesRead++;
                    if (startReading) {
                        Serial.print((char)c);
                    }

                    if (c >= 0) {
                        if (c == '\r')  {
                            // Ignore
                        }
                        else if (!startReading) {
                            if (c == '\n') {
                                startReading = true;
                            }
                        }
                        else if (c == ',' || c == '\n') {
                            if (length > 0) {
                                buffer[length] = 0;
                                if (numberOfValues < FK_WEATHER_STATION_MAX_VALUES) {
                                    values[numberOfValues] = atof(buffer);
                                    numberOfValues++;
                                }
                                length = 0;;
                            }
                            if (c == '\n') {
                                bool success = numberOfValues == FK_WEATHER_STATION_PACKET_NUMBER_VALUES;
                                if (success) {
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

                                        if (!clockSet) {
                                            SystemClock->set(fix.time);
                                            clockSet = true;
                                        }
                                    }
                                    else {
                                        // There's a good chance our RTC has a good
                                        // time, so just insert this so that logged
                                        // entries don't have 0s.
                                        values[FK_WEATHER_STATION_FIELD_UNIXTIME] = SystemClock->now();
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
                                        checkingCommunications = false;
                                        transition(WeatherStationState::CommunicationsOk);
                                    }
                                    else {
                                        logReadingLocally();

                                        DEBUG_PRINT("WS: >Waiting ms=");
                                        DEBUG_PRINT(platformUptime() - started);
                                        DEBUG_PRINT(" ");
                                        DEBUG_PRINTLN(bytesRead);
                                        transition(WeatherStationState::Waiting);
                                    }
                                }
                                else {
                                    if (true) {
                                        DEBUG_PRINT("WS: no reading: ");
                                        DEBUG_PRINT(numberOfValues);
                                        DEBUG_PRINT(" ");
                                        DEBUG_PRINT(FK_WEATHER_STATION_PACKET_NUMBER_VALUES);
                                        DEBUG_PRINTLN("");
                                    }
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
        }

        WeatherSerial.end();

        break;
    }
    }

    return false;
}

void WeatherStation::logReadingLocally() {
    File file = Logger::open(FK_SETTINGS_WEATHER_STATION_DATA_FILENAME);
    if (file) {
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

    Queue queue;

    weather_station_packet_t packet;
    memzero((uint8_t *)&packet, sizeof(weather_station_packet_t));
    packet.fk.kind = FK_PACKET_KIND_WEATHER_STATION;
    packet.time = SystemClock->now();
    packet.battery = gauge->stateOfCharge();
    for (uint8_t i = 0; i < FK_WEATHER_STATION_PACKET_NUMBER_VALUES; ++i) {
        packet.values[i] = values[i];
    }

    queue.enqueue((uint8_t *)&packet, sizeof(weather_station_packet_t));
}

void WeatherStation::transition(WeatherStationState newState) {
    state = newState;
    lastTransitionAt = platformUptime();
}

#endif
