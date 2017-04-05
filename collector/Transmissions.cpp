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

Transmissions::Transmissions(CorePlatform *core, WeatherStation *weatherStation, RtcAbstractSystemClock *systemClock, Configuration *configuration,
                             TransmissionStatus *status, FuelGauge *fuel, Memory *memory) :
    core(core), weatherStation(weatherStation), systemClock(systemClock), configuration(configuration), status(status), fuel(fuel), memory(memory) {
}

String Transmissions::atlasPacketToMessage(atlas_sensors_packet_t *packet) {
    String message(packet->time);
    message += ",";
    message += configuration->getName();
    message += ",AT";
    message += "," + String(fuel->cellVoltage(), 2);
    message += "," + String(fuel->stateOfCharge(), 2);
    message += "," + String(packet->battery, 2);
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
    message += ",SO";
    message += "," + String(fuel->cellVoltage(), 2);
    message += "," + String(fuel->stateOfCharge(), 2);
    message += "," + String(packet->battery, 2);
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
    message += ",WE";
    message += "," + String(fuel->cellVoltage(), 2);
    message += "," + String(fuel->stateOfCharge(), 2);
    message += "," + String(packet->battery, 2);
    uint8_t fields[] = {
        FK_WEATHER_STATION_FIELD_TEMPERATURE_OUTSIDE,
        FK_WEATHER_STATION_FIELD_HUMIDITY_OUTSIDE,
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
    return message;
}

void Transmissions::sendSensorTransmission(bool sendAtlas, bool sendWeather, bool sendSonar) {
    Queue keeping("KEEPING.BIN");
    Queue queue;

    uint32_t queueSize = queue.size();

    DEBUG_PRINT("TS: sendSensorTransmission: ");
    DEBUG_PRINT(queueSize);
    if (sendAtlas) DEBUG_PRINT(" atlas");
    if (sendSonar) DEBUG_PRINT(" sonar");
    if (sendWeather) DEBUG_PRINT(" weather");
    DEBUG_PRINTLN();

    if (queueSize == 0) {
        return;
    }

    atlas_sensors_packet_t atlas_station_sensors;
    memzero((uint8_t *)&atlas_station_sensors, sizeof(atlas_sensors_packet_t));

    weather_station_packet_t weather_station_sensors;
    memzero((uint8_t *)&weather_station_sensors, sizeof(weather_station_packet_t));

    sonar_station_packet_t sonar_station_sensors;
    memzero((uint8_t *)&sonar_station_sensors, sizeof(sonar_station_packet_t));

    while (true) {
        fk_network_packet_t *packet = (fk_network_packet_t *)queue.dequeue();
        if (packet == NULL) {
            break;
        }

        switch (packet->kind) {
        case FK_PACKET_KIND_WEATHER_STATION: {
            if (sendWeather) {
                memcpy((uint8_t *)&weather_station_sensors, (uint8_t *)packet, sizeof(weather_station_packet_t));
            }
            else {
                keeping.enqueue((uint8_t *)packet);
            }
            break;
        }
        case FK_PACKET_KIND_ATLAS_SENSORS: {
            if (sendAtlas) {
                memcpy((uint8_t *)&atlas_station_sensors, (uint8_t *)packet, sizeof(atlas_sensors_packet_t));
            }
            else {
                keeping.enqueue((uint8_t *)packet);
            }
            break;
        }
        case FK_PACKET_KIND_SONAR_STATION: {
            if (sendSonar){
                memcpy((uint8_t *)&sonar_station_sensors, (uint8_t *)packet, sizeof(sonar_station_packet_t));
            }
            else {
                keeping.enqueue((uint8_t *)packet);
            }
            break;
        }
        }
    }

    keeping.startAtBeginning();
    queue.startAtBeginning();

    keeping.copyInto(&queue);
    keeping.removeAll();

    if (sendAtlas) {
        if (atlas_station_sensors.fk.kind == FK_PACKET_KIND_ATLAS_SENSORS) {
            transmission(atlasPacketToMessage(&atlas_station_sensors));
        }
        else {
            diagnostics.recordTransmissionSkipped();
        }
    }

    if (sendSonar) {
        if (sonar_station_sensors.fk.kind == FK_PACKET_KIND_SONAR_STATION) {
            transmission(sonarPacketToMessage(&sonar_station_sensors));
        }
        else {
            diagnostics.recordTransmissionSkipped();
        }
    }

    if (sendWeather) {
        if (weather_station_sensors.fk.kind == FK_PACKET_KIND_WEATHER_STATION) {
            transmission(weatherStationPacketToMessage(&weather_station_sensors));
        }
        else {
            diagnostics.recordTransmissionSkipped();
        }
    }
}

String Transmissions::locationToMessage(gps_fix_t *location) {
    uint32_t uptime = diagnostics.getUptime();
    String message(location->time);
    message += ",";
    message += configuration->getName();
    message += ",LO";
    message += "," + String(fuel->cellVoltage(), 2);
    message += "," + String(fuel->stateOfCharge(), 2);
    message += "," + String(location->latitude, 6);
    message += "," + String(location->longitude, 6);
    message += "," + String(location->altitude, 2);
    message += "," + String(uptime);
    return message;
}

void Transmissions::sendLocationTransmission() {
    if (weatherStation->getFix()->time > 0) {
        transmission(locationToMessage(weatherStation->getFix()));
    }
}

String Transmissions::diagnosticsToMessage() {
    uint32_t uptime = diagnostics.getUptime();
    String message(SystemClock->now());
    message += ",";
    message += configuration->getName();
    message += ",ST";
    message += "," + String(fuel->cellVoltage(), 2);
    message += "," + String(fuel->stateOfCharge(), 2);
    message += "," + String(core->isSdAvailable());
    message += "," + String(diagnostics.communicationsPassed);
    message += "," + String(diagnostics.weatherStationPassed );
    message += "," + String(diagnostics.loraPassed );
    message += "," + String(diagnostics.hasGpsFix);
    message += "," + String(diagnostics.batterySleepTime);
    message += "," + String(platformAdjustUptime(0));
    message += "," + String(diagnostics.numberOfTransmissionFailures);
    message += "," + String(diagnostics.numberOfTransmissionSkipped);
    message += "," + String(diagnostics.weatherReadingsReceived);
    message += "," + String(diagnostics.atlasPacketsReceived);
    message += "," + String(diagnostics.sonarPacketsReceived);
    message += "," + String(diagnostics.deadFor);
    message += "," + String(diagnostics.getAverageTransmissionTime(), 2);

    fk_memory_core_intervals_t *intervals = memory->intervals();

    message += "," + String(intervalToMs(intervals->idle));
    message += "," + String(intervalToMs(intervals->airwaves));
    message += "," + String(intervalToMs(intervals->restart));
    message += "," + String(intervals->weatherStation.stop);

    fk_transmission_schedule_t *schedules = memory->schedules();

    for (uint8_t i = 0; i < TRANSMISSION_KIND_KINDS; ++i) {
        message += "," + String(schedules[i].offset);
        message += "," + String(schedules[i].interval);
    }

    message += "," + String(uptime);
    return message;
}

bool Transmissions::sendStatusTransmission() {
    return transmission(diagnosticsToMessage());
}

void Transmissions::handleTransmissionIfNecessary() {
    TransmissionStatus status;

    int8_t kind = status.shouldWe(memory->schedules(), false);
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
        if (false) {
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
        if (true) {
            RockBlock rockBlock(this, (uint8_t *)message.c_str(), message.length());
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

    diagnostics.recordTransmission(millis() - started);

    if (!success) {
        diagnostics.recordTransmissionFailure();
    }

    if (forceStatus) {
        forceStatus = false;
        sendStatusTransmission();
    }

    digitalWrite(PIN_RED_LED, LOW);

    return success;
}

bool message_parse_csv(String message, uint8_t numberExpected, uint32_t *values) {
    uint8_t index = 0;
    bool valid = true;

    for (int8_t start = message.indexOf(",") + 1; start <= message.length() && index < numberExpected; ) {
        int8_t nextComma = message.indexOf(",", start);
        int8_t end = nextComma >= 0 ? nextComma : message.length();
        int32_t value = atoi(message.substring(start, end).c_str());
        if (value < 0) {
            valid = false;
            break;
        }
        else {
            values[index++] = value;
        }
        start = end + 1;
    }

    return valid && index == numberExpected;
}

void Transmissions::onMessage(String message) {
    // Defaults: IV,600000,600000,10000,21600000,30
    if (message.startsWith("IV,")) {
        const uint8_t numberOfValues = 7;
        uint32_t values[numberOfValues] = { 0 };
        bool valid = true;

        if (message_parse_csv(message, numberOfValues, values)) {
            if (valid) {
                fk_memory_core_intervals_t *intervals = memory->intervals();

                intervals->idle = msToInterval(values[0]);
                intervals->airwaves = msToInterval(values[1]);
                intervals->restart = msToInterval(values[2]);
                intervals->weatherStation.stop = values[3];

                DEBUG_PRINT("New Intervals:");

                for (uint8_t i = 0; i < numberOfValues; ++i) {
                    DEBUG_PRINT(" ");
                    DEBUG_PRINT(values[i]);
                }
                DEBUG_PRINTLN("");

                forceStatus = true;
            }
        }
        else {
            valid = false;
        }

        if (!valid) {
            DEBUG_PRINTLN("Ignored invalid intervals.");
        }
    }

    // Defaults: SC,24,24,0,6,2,6
    if (message.startsWith("SC,")) {
        const uint8_t numberOfValues = TRANSMISSION_KIND_KINDS * 2;
        uint32_t values[numberOfValues] = { 0 };

        if (message_parse_csv(message, numberOfValues, values)) {
            fk_transmission_schedule_t *schedules = memory->schedules();

            DEBUG_PRINTLN("New Schedule: ");

            uint8_t index = 0;
            for (uint8_t i = 0; i < TRANSMISSION_KIND_KINDS; ++i) {
                schedules[i].offset = values[index++];
                schedules[i].interval = values[index++];

                DEBUG_PRINT(schedules[i].offset);
                DEBUG_PRINT(" ");
                DEBUG_PRINT(schedules[i].interval);
                DEBUG_PRINTLN("");
            }

            forceStatus = true;
        }
        else {
            DEBUG_PRINTLN("Ignored invalid schedule.");
        }
    }
}
