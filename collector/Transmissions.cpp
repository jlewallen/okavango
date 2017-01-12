#include <Adafruit_SleepyDog.h>
#include "Transmissions.h"
#include "TransmissionStatus.h"
#include "fona.h"
#include "RockBlock.h"
#include "Queue.h"
#include "Diagnostics.h"
#ifdef BUILD_JOB
#define NUMBER_TO_SMS ""
#else
#include "config.h"
#endif

Transmissions::Transmissions(WeatherStation *weatherStation, RtcAbstractSystemClock *systemClock, Configuration *configuration, TransmissionStatus *status) :
    weatherStation(weatherStation), systemClock(systemClock), configuration(configuration), status(status) {
}

String Transmissions::atlasPacketToMessage(atlas_sensors_packet_t *packet) {
    String message(packet->time);
    message += ",";
    message += configuration->getName();
    message += "," + String(platformBatteryVoltage(), 2);
    message += "," + String(platformBatteryLevel(), 2);
    message += ",";
    message += String(packet->battery, 2);
    for (uint8_t i = 0; i < FK_ATLAS_SENSORS_PACKET_NUMBER_VALUES; ++i) {
        String field = "," + String(packet->values[i], 2);
        message += field;
    }
    return message;
}

String Transmissions::sonarPacketToMessage(sonar_station_packet_t *packet) {
    String message(packet->time);
    message += ",";
    message += configuration->getName();
    message += "," + String(platformBatteryVoltage(), 2);
    message += "," + String(platformBatteryLevel(), 2);
    message += ",";
    message += String(packet->battery, 2);
    for (uint8_t i = 0; i < FK_SONAR_STATION_PACKET_NUMBER_VALUES; ++i) {
        String field = "," + String(packet->values[i], 2);
        message += field;
    }
    return message;
}

String Transmissions::weatherStationPacketToMessage(weather_station_packet_t *packet) {
    String message(packet->time);
    message += ",";
    message += configuration->getName();
    message += "," + String(platformBatteryVoltage(), 2);
    message += "," + String(platformBatteryLevel(), 2);
    uint8_t fields[] = {
        FK_WEATHER_STATION_FIELD_TEMPERATURE,
        FK_WEATHER_STATION_FIELD_HUMIDITY,
        FK_WEATHER_STATION_FIELD_PRESSURE,
        FK_WEATHER_STATION_FIELD_WIND_SPEED_2M,
        FK_WEATHER_STATION_FIELD_WIND_DIR_2M,
        FK_WEATHER_STATION_FIELD_WIND_GUST_10M,
        FK_WEATHER_STATION_FIELD_WIND_GUST_DIR_10m,
        FK_WEATHER_STATION_FIELD_DAILY_RAIN
    };
    for (uint8_t i = 0; i < sizeof(fields) / sizeof(uint8_t); ++i) {
        String field = "," + String(packet->values[fields[i]], 2);
        message += field;
    }
    message += ",";
    message += String(packet->battery, 2);
    return message;
}

void Transmissions::sendSensorTransmission(bool sendAtlas, bool sendWeather, bool sendSonar) {
    Queue queue;

    uint32_t queueSize = queue.size();

    DEBUG_PRINT("TS: sendSensorTransmission: ");
    DEBUG_PRINT(queueSize);
    if (sendAtlas) DEBUG_PRINT(" atlas");
    if (sendSonar) DEBUG_PRINT(" sonar");
    if (sendWeather) DEBUG_PRINT(" weather");
    DEBUG_PRINTLN();

    atlas_sensors_packet_t atlas_station_sensors;
    memzero((uint8_t *)&atlas_station_sensors, sizeof(atlas_sensors_packet_t));

    weather_station_packet_t weather_station_sensors;
    memzero((uint8_t *)&weather_station_sensors, sizeof(weather_station_packet_t));

    sonar_station_packet_t sonar_station_sensors;
    memzero((uint8_t *)&sonar_station_sensors, sizeof(sonar_station_packet_t));

    if (queueSize > 0) {
        while (true) {
            fk_network_packet_t *packet = (fk_network_packet_t *)queue.dequeue();
            if (packet == NULL) {
                break;
            }

            switch (packet->kind) {
            case FK_PACKET_KIND_WEATHER_STATION: {
                memcpy((uint8_t *)&weather_station_sensors, (uint8_t *)packet, sizeof(weather_station_packet_t));
                break;
            }
            case FK_PACKET_KIND_ATLAS_SENSORS: {
                memcpy((uint8_t *)&atlas_station_sensors, (uint8_t *)packet, sizeof(atlas_sensors_packet_t));
                break;
            }
            case FK_PACKET_KIND_SONAR_STATION: {
                memcpy((uint8_t *)&sonar_station_sensors, (uint8_t *)packet, sizeof(sonar_station_packet_t));
                break;
            }
            }
        }
    }

    bool noAtlas = false;
    bool noWeather = false;
    bool noSonar = false;

    if (sendAtlas) {
        if (atlas_station_sensors.fk.kind == FK_PACKET_KIND_ATLAS_SENSORS) {
            if (transmission(atlasPacketToMessage(&atlas_station_sensors))) {
            }
        }
        else {
            noAtlas = true;
        }
    }

    if (sendSonar) {
        if (sonar_station_sensors.fk.kind == FK_PACKET_KIND_SONAR_STATION) {
            if (transmission(sonarPacketToMessage(&sonar_station_sensors))) {
            }
        }
        else {
            noSonar = true;
        }
    }

    if (sendWeather) {
        if (weather_station_sensors.fk.kind == FK_PACKET_KIND_WEATHER_STATION) {
            if (transmission(weatherStationPacketToMessage(&weather_station_sensors))) {
            }
        }
        else {
            noWeather = true;
        }
    }

    /*
    if (triggered) {
        if ((noAtlas && noSonar) || noWeather) {
            uint32_t uptime = millis() / (1000 * 60);
            String message(systemClock->now());
            message += ",";
            message += configuration->getName();
            message += "," + String(platformBatteryVoltage(), 2);
            message += "," + String(platformBatteryLevel(), 2);
            message += ",";
            message += noAtlas;
            message += ",";
            message += noSonar;
            message += ",";
            message += noWeather;
            message += ",";
            message += queueSize;
            message += ",";
            message += diagnostics.numberOfTransmissionFailures;
            message += ",";
            message += uptime;
            transmission(message);
        }
    }
    */
}

String Transmissions::locationToMessage(gps_fix_t *location) {
    uint32_t uptime = millis() / (1000 * 60);
    String message(location->time);
    message += ",";
    message += configuration->getName();
    message += "," + String(platformBatteryVoltage(), 2);
    message += "," + String(platformBatteryLevel(), 2);
    message += "," + String(location->latitude, 6);
    message += "," + String(location->longitude, 6);
    message += "," + String(location->altitude, 2);
    message += ",";
    message += uptime;
    return message;
}

void Transmissions::sendLocationTransmission() {
    if (weatherStation->getFix()->time > 0) {
        if (transmission(locationToMessage(weatherStation->getFix()))) {
        }
    }
}

void Transmissions::sendStatusTransmission() {
}

void Transmissions::handleTransmissionIfNecessary() {
    TransmissionStatus status;

    int8_t kind = status.shouldWe();
    if (kind == TRANSMISSION_KIND_SENSORS) {
        sendSensorTransmission(true, false, true);
    }
    else if (kind == TRANSMISSION_KIND_WEATHER) {
        sendSensorTransmission(false, true, false);
    }
    else if (kind == TRANSMISSION_KIND_LOCATION) {
        sendLocationTransmission();
    }
}

bool Transmissions::transmission(String message) {
    DEBUG_PRINT("Message: ");
    DEBUG_PRINTLN(message.c_str());

    digitalWrite(PIN_RED_LED, HIGH);

    bool success = false;
    uint32_t started = millis();
    if (message.length() > 0) {
        if (configuration->hasFonaAttached()) {
            FonaChild fona(NUMBER_TO_SMS, message);
            platformSerial2Begin(9600);
            SerialType &fonaSerial = Serial2;
            fona.setSerial(&fonaSerial);
            while (!fona.isDone() && !fona.isFailed()) {
                if (millis() - started < THIRTY_MINUTES) {
                    Watchdog.reset();
                }
                weatherStation->tick();
                fona.tick();
                delay(10);
            }
            success = fona.isDone();
            DEBUG_PRINT("Fona: ");
            DEBUG_PRINTLN(success);
        }
        if (configuration->hasRockBlockAttached()) {
            RockBlock rockBlock(message);
            rockBlockSerialBegin();
            SerialType &rockBlockSerial = RockBlockSerial;
            rockBlock.setSerial(&rockBlockSerial);
            while (!rockBlock.isDone() && !rockBlock.isFailed()) {
                if (millis() - started < THIRTY_MINUTES) {
                    Watchdog.reset();
                }
                weatherStation->tick(); // Remember with the IridiumSBD library
                                        // this loop is only happens once.
                rockBlock.tick();
                delay(10);
            }
            success = rockBlock.isDone();
            DEBUG_PRINT("RockBlock: ");
            DEBUG_PRINTLN(success);
        }
    }

    if (!success) {
        diagnostics.recordTransmissionFailure();
    }

    digitalWrite(PIN_RED_LED, LOW);

    return success;
}
