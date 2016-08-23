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
#include <Adafruit_SleepyDog.h>

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
bool transmissionForced = false;
bool initialWeatherTransmissionSent = false;
bool initialAtlasTransmissionSent = true;
bool initialLocationTransmissionSent = false;

#define MANDATORY_RESTART_INTERVAL   (1000 * 60 * 60 * 3)
#define MANDATORY_RESTART_FILE       "RESUME.INF"

class CollectorNetworkCallbacks : public NetworkCallbacks {
    virtual bool forceTransmission(NetworkProtocolState *networkProtocol) {
        transmissionForced = true;
        return true;
    }
};

class SelfRestart {
public:
    static void restartIfNecessary() {
        if (millis() > MANDATORY_RESTART_INTERVAL) {
            File file = SD.open(MANDATORY_RESTART_FILE, FILE_WRITE);
            if (!file) {
                // Consider not doing the restart now? Maybe waiting another interval?
                DEBUG_PRINTLN("Error creating restart indicator file.");
            }
            else {
                file.close();
            }
            DEBUG_PRINTLN("Mandatory restart triggered.");
            platformRestart();
        }
    }

    static bool didWeJustRestart() {
        if (SD.exists(MANDATORY_RESTART_FILE)) {
            SD.remove(MANDATORY_RESTART_FILE);
            return true;
        }
        return false;
    }
};

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

    #ifdef FK_WRITE_LOG_FILE
    logPrinter.open();
    #endif

    if (!configuration.read()) {
        DEBUG_PRINTLN("Error reading configuration");
        platformCatastrophe(PIN_RED_LED);
    }

    if (configuration.hasRockBlockAttached()) {
        pinMode(PIN_ROCK_BLOCK, OUTPUT);
        digitalWrite(PIN_ROCK_BLOCK, LOW);
    }

    if (SelfRestart::didWeJustRestart()) {
        DEBUG_PRINTLN("Resume from mandatory restart.");
        initialWeatherTransmissionSent = true;
        initialAtlasTransmissionSent = true;
        initialLocationTransmissionSent = true;
    }

    memzero((uint8_t *)&location, sizeof(gps_location_t));

    DEBUG_PRINTLN(F("Loop"));
}

void checkAirwaves() {
    Queue queue;
    LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST);
    NetworkProtocolState networkProtocol(NetworkState::EnqueueFromNetwork, &radio, &queue, new CollectorNetworkCallbacks());
    WeatherStation weatherStation;

    DEBUG_PRINTLN("Checking Airwaves...");

    int32_t watchdogMs = Watchdog.enable();

    DEBUG_PRINT("Watchdog enabled: ");
    DEBUG_PRINTLN(watchdogMs);

    weatherStation.setup();

    // Can't call this more than 3 times or so because we use up all the IRQs and
    // so this would be nice to have a kind of memory?
    if (!radioSetup) {
        if (radio.setup()) {
            radio.sleep();
        }
        else {
            platformCatastrophe(PIN_RED_LED);
        }
        radioSetup = true;
    }

    uint32_t started = millis();
    uint32_t last = 0;
    while (true) {
        Watchdog.reset();

        networkProtocol.tick();

        weatherStation.ignore();

        delay(10);

        if (millis() - last > 5000) {
            platformBlink(PIN_RED_LED);
            DEBUG_PRINT(".");
            last = millis();

            SelfRestart::restartIfNecessary();
        }

        if (millis() - started > 2 * 60 * 1000 && networkProtocol.isQuiet()) {
            break;
        }

        if (transmissionForced) {
            break;
        }

    }

    radio.sleep();

    #ifdef FK_WRITE_LOG_FILE
    logPrinter.flush();
    #endif

    DEBUG_PRINTLN("");

    started = millis();
    bool success = false;
    weatherStation.clear();

    while (millis() - started < 10 * 1000) {
        if (weatherStation.tick()) {
            DEBUG_PRINTLN("");

            DEBUG_PRINT("&");

            weatherStation.logReadingLocally();

            float *values = weatherStation.getValues();
            DEBUG_PRINT("%");
            SystemClock.set((uint32_t)values[FK_WEATHER_STATION_FIELD_UNIXTIME]);

            DEBUG_PRINT("%");

            location.time = values[FK_WEATHER_STATION_FIELD_UNIXTIME];
            location.latitude = values[FK_WEATHER_STATION_FIELD_LATITUDE];
            location.longitude = values[FK_WEATHER_STATION_FIELD_LONGITUDE];
            location.altitude = values[FK_WEATHER_STATION_FIELD_ALTITUDE];
            location.satellites = values[FK_WEATHER_STATION_FIELD_SATELLITES];

            DEBUG_PRINT("%");

            weather_station_packet_t packet;
            memzero((uint8_t *)&packet, sizeof(weather_station_packet_t));
            packet.fk.kind = FK_PACKET_KIND_WEATHER_STATION;
            packet.time = SystemClock.now();
            packet.battery = platformBatteryVoltage();
            for (uint8_t i = 0; i < FK_WEATHER_STATION_PACKET_NUMBER_VALUES; ++i) {
                packet.values[i] = values[i];
            }

            DEBUG_PRINT("%");

            queue.enqueue((uint8_t *)&packet);

            weatherStation.clear();
            DEBUG_PRINT("^");

            success = true;

            break;
        }
    }

    if (!success) {
        DEBUG_PRINTLN("Unable to get Weather Station reading.");
    }

    weatherStation.off();

    #ifdef FK_WRITE_LOG_FILE
    logPrinter.flush();
    #endif

    Watchdog.disable();
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

bool singleTransmission(String message) {
    DEBUG_PRINT("Message: ");
    DEBUG_PRINTLN(message.c_str());

    bool success = false;
    uint32_t started = millis();
    int32_t watchdogMs = Watchdog.enable();
    if (message.length() > 0) {
        if (configuration.hasFonaAttached()) {
            FonaChild fona(NUMBER_TO_SMS, message);
            Serial1.begin(9600);
            SerialType &fonaSerial = Serial1;
            fona.setSerial(&fonaSerial);
            while (!fona.isDone() && !fona.isFailed()) {
                if (millis() - started < THIRTY_MINUTES) {
                    Watchdog.reset();
                }
                fona.tick();
                delay(10);
            }
            success = fona.isDone();
            DEBUG_PRINT("Fona: ");
            DEBUG_PRINTLN(success);
        }
        if (configuration.hasRockBlockAttached()) {
            RockBlock rockBlock(message);
            Serial1.begin(19200);
            SerialType &rockBlockSerial = Serial1;
            rockBlock.setSerial(&rockBlockSerial);
            while (!rockBlock.isDone() && !rockBlock.isFailed()) {
                if (millis() - started < THIRTY_MINUTES) {
                    Watchdog.reset();
                }
                rockBlock.tick();
                delay(10);
            }
            success = rockBlock.isDone();
            DEBUG_PRINT("RockBlock: ");
            DEBUG_PRINTLN(success);
        }
    }
    Watchdog.disable();

    return success;
}

void handleSensorTransmission() {
    Queue queue;

    if (queue.size() <= 0) {
        DEBUG_PRINTLN("Queue empty");
        return;
    }

    Watchdog.enable();

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
        if (singleTransmission(atlasPacketToMessage(&atlas_sensors))) {
            initialAtlasTransmissionSent = true;
        }
    }

    if (weather_station_sensors.fk.kind == FK_PACKET_KIND_WEATHER_STATION) {
        if (singleTransmission(weatherStationPacketToMessage(&weather_station_sensors))) {
            initialWeatherTransmissionSent = true;
        }
    }

    Watchdog.disable();
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
        if (singleTransmission(locationToMessage(&location))) {
            initialLocationTransmissionSent = true;
        }
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

    if (transmissionForced) {
        handleSensorTransmission();
        handleLocationTransmission();
        transmissionForced = false;
    }

    if (!initialWeatherTransmissionSent || !initialAtlasTransmissionSent) {
        handleSensorTransmission();
    }
    if (!initialLocationTransmissionSent) {
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
