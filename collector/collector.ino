#include "Platforms.h"
#include "core.h"
#include "protocol.h"
#include "network.h"
#include "fona.h"
#include "RockBlock.h"
#include "config.h"
#include "TransmissionStatus.h"
#include "WeatherStation.h"
#include "Configuration.h"

#define FONA

bool radioSetup = false;

typedef struct gps_location_t {
    float latitude;
    float longitude;
    float altitude;
    uint8_t satellites;
    uint32_t time;
} gps_location_t;

Configuration configuration(FK_SETTINGS_CONFIGURATION_FILENAME);
gps_location_t location;

void setup() {
    platformLowPowerSleep(LOW_POWER_SLEEP_BEGIN);

    Serial.begin(115200);

    #ifdef WAIT_FOR_SERIAL
    while (!Serial) {
        delay(100);
        if (millis() > WAIT_FOR_SERIAL) {
            break;
        }
    }
    #endif

    Serial.println(F("Begin"));

    CorePlatform corePlatform;
    corePlatform.setup();

    if (!configuration.read()) {
        Serial.println("Error reading configuration");
        platformCatastrophe(PIN_RED_LED);
    }

    memzero((uint8_t *)&location, sizeof(gps_location_t));

    Serial.println(F("Loop"));
}

void checkAirwaves() {
    Queue queue;
    LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST);
    NetworkProtocolState networkProtocol(NetworkState::EnqueueFromNetwork, &radio, &queue);
    WeatherStation weatherStation;

    Serial.println("Checking Airwaves...");
    
    weatherStation.setup();

    // Can't call this more than 3 times or so because we use up all the IRQs and
    // so this would be nice to have a kind of memory?
    if (!radioSetup) {
        if (radio.setup()) {
            radio.sleep();
        }
        radioSetup = true;
    }

    uint32_t started = millis();
    uint32_t last = 0;
    while (true) {
        networkProtocol.tick();

        weatherStation.ignore();

        delay(10);

        if (millis() - last > 5000) {
            Serial.print(".");
            last = millis();
        }

        if (millis() - started > 2 * 60 * 1000 && networkProtocol.isQuiet()) {
            break;
        }

    }

    radio.sleep();

    Serial.println();

    started = millis();
    bool success = false;
    weatherStation.clear();

    while (millis() - started < 10 * 1000) {
        if (weatherStation.tick()) {
            Serial.println();

            Serial.print("&");

            weatherStation.logReadingLocally();

            float *values = weatherStation.getValues();
            SystemClock.set((uint32_t)values[FK_WEATHER_STATION_FIELD_UNIXTIME]);

            location.time = values[FK_WEATHER_STATION_FIELD_UNIXTIME];
            location.latitude = values[FK_WEATHER_STATION_FIELD_LATITUDE];
            location.longitude = values[FK_WEATHER_STATION_FIELD_LONGITUDE];
            location.altitude = values[FK_WEATHER_STATION_FIELD_ALTITUDE];
            location.satellites = values[FK_WEATHER_STATION_FIELD_SATELLITES];

            weather_station_packet_t packet;
            memzero((uint8_t *)&packet, sizeof(weather_station_packet_t));
            packet.fk.kind = FK_PACKET_KIND_WEATHER_STATION;
            packet.time = SystemClock.now();
            packet.battery = platformBatteryVoltage();
            for (uint8_t i = 0; i < FK_WEATHER_STATION_PACKET_NUMBER_VALUES; ++i) {
                packet.values[i] = values[i];
            }

            queue.enqueue((uint8_t *)&packet);

            weatherStation.clear();
            Serial.print("^");

            success = true;

            break;
        }
    }

    if (!success) {
        Serial.println("Unable to get Weather Station reading.");
    }

    weatherStation.off();
}

String atlasPacketToMessage(atlas_sensors_packet_t *packet) {
    String message(packet->time);
    message += ",";
    message += configuration.getName();
    message += ",";
    message += String(packet->battery, 2);
    for (uint8_t i = 0; i < FK_ATLAS_SENSORS_PACKET_NUMBER_VALUES; ++i) {
        String field = "," + String(packet->values[i], 2);
        message += field;
    }
    return message;
}

String weatherStationPacketToMessage(weather_station_packet_t *packet) {
    String message(packet->time);
    message += ",";
    message += configuration.getName();
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

void singleTransmission(String message) {
    Serial.print("Message: ");
    Serial.println(message);
    Serial.println(message.length());

    if (message.length() > 0) {
        if (configuration.hasFonaAttached()) {
            FonaChild fona(NUMBER_TO_SMS, message);
            Serial1.begin(4800);
            SerialType &fonaSerial = Serial1;
            fona.setSerial(&fonaSerial);
            while (!fona.isDone() && !fona.isFailed()) {
                fona.tick();
                delay(10);
            }
        }
        if (configuration.hasRockBlockAttached()) {
            RockBlock rockBlock(message);
            Serial1.begin(19200);
            SerialType &rockBlockSerial = Serial1;
            rockBlock.setSerial(&rockBlockSerial);
            while (!rockBlock.isDone() && !rockBlock.isFailed()) {
                rockBlock.tick();
                delay(10);
            }
        }
    }
}

void handleSensorTransmission() {
    Queue queue;

    if (queue.size() <= 0) {
        DEBUG_PRINTLN("Queue empty");
        return;
    }

    atlas_sensors_packet_t atlas_sensors;
    memzero((uint8_t *)&atlas_sensors, sizeof(atlas_sensors_packet_t));

    weather_station_packet_t weather_station_sensors;
    memzero((uint8_t *)&weather_station_sensors, sizeof(weather_station_packet_t));

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
            memcpy((uint8_t *)&atlas_sensors, (uint8_t *)packet, sizeof(atlas_sensors_packet_t));
            break;
        }
        }
    }

    if (atlas_sensors.fk.kind == FK_PACKET_KIND_ATLAS_SENSORS) {
        singleTransmission(atlasPacketToMessage(&atlas_sensors));
    }

    if (weather_station_sensors.fk.kind == FK_PACKET_KIND_WEATHER_STATION) {
        singleTransmission(weatherStationPacketToMessage(&weather_station_sensors));
    }
}

String locationToMessage(gps_location_t *location) {
    String message(location->time);
    message += ",";
    message += configuration.getName();
    message += "," + String(location->latitude, 6);
    message += "," + String(location->longitude, 6);
    message += "," + String(location->altitude, 2);
    return message;
}

void handleLocationTransmission() {
    if (location.time > 0) {
        singleTransmission(locationToMessage(&location));
    }
}

void handleTransmissionIfNecessary() {
    TransmissionStatus status;
    int8_t kind = status.shouldWe();
    if (kind == TRANSMISSION_KIND_SENSORS) {
        handleSensorTransmission();
    }
    else if (kind == TRANSMISSION_KIND_LOCATION) {
        handleLocationTransmission();
    }
}

typedef enum CollectorState {
    Airwaves,
    WeatherStation,
    Transmission
} CollectorState;

void loop() {
    CollectorState state = Airwaves;

    while (1) {
        switch (state) {
        case Airwaves: {
            checkAirwaves();
            state = Transmission;
            break;
        }
        case WeatherStation: {
            break;
        }
        case Transmission: {
            handleTransmissionIfNecessary();
            state = Airwaves;
            break;
        }
        }
    }
}

// vim: set ft=cpp:
