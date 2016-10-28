#include <Adafruit_SleepyDog.h>
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
#include "UptimeTracker.h"
#include "InitialTransmissions.h"
#include "system.h"

void logTransition(const char *name);

typedef struct gps_location_t {
    float latitude;
    float longitude;
    float altitude;
    uint8_t satellites;
    uint32_t time;
} gps_location_t;

Configuration configuration(FK_SETTINGS_CONFIGURATION_FILENAME);
WeatherStation weatherStation;
gps_location_t location;
uint32_t numberOfFailures = 0;
bool transmissionForced = false;
bool initialWeatherTransmissionSent = false;
bool initialAtlasTransmissionSent = false;
bool initialSonarTransmissionSent = false;
bool initialLocationTransmissionSent = false;
bool radioSetup = false;

#define IDLE_PERIOD                  (1000 * 60 * 2)
#define AIRWAVES_CHECK_TIME          (1000 * 60 * 2)
#define WEATHER_STATION_CHECK_TIME   (1000 * 10)
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
            logPrinter.flush();
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
    Watchdog.enable();

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

    Watchdog.reset();

    CorePlatform corePlatform;
    corePlatform.setup();

    UptimeTracker::started();

    logPrinter.open();

    switch (system_get_reset_cause()) {
    case SYSTEM_RESET_CAUSE_SOFTWARE: logPrinter.println("ResetCause: Software"); break;
    case SYSTEM_RESET_CAUSE_WDT: logPrinter.println("ResetCause: WDT"); break;
    case SYSTEM_RESET_CAUSE_EXTERNAL_RESET: logPrinter.println("ResetCause: External Reset"); break;
    case SYSTEM_RESET_CAUSE_BOD33: logPrinter.println("ResetCause: BOD33"); break;
    case SYSTEM_RESET_CAUSE_BOD12: logPrinter.println("ResetCause: BOD12"); break;
    case SYSTEM_RESET_CAUSE_POR: logPrinter.println("ResetCause: PoR"); break;
    }

    #ifndef TESTING_MODE
    if (UptimeTracker::shouldWeRelax()) {
        DEBUG_PRINTLN("Relaxing");
        logPrinter.flush();

        // I would love to be able to reliably tell if we're charging now, but
        // that may be too much of a hardware change.
        uint32_t relaxingAt = millis();
        while (millis() - relaxingAt < FIVE_MINUTES) {
            Watchdog.reset();
            delay(1000);
        }
        platformRestart();
    }
    #endif

    logTransition("Begin");
    logPrinter.flush();

    if (!configuration.read()) {
        DEBUG_PRINTLN("Error reading configuration");
        logPrinter.flush();
        platformCatastrophe(PIN_RED_LED);
    }

    if (configuration.hasRockBlockAttached()) {
        pinMode(PIN_ROCK_BLOCK, OUTPUT);
        digitalWrite(PIN_ROCK_BLOCK, LOW);
    }

    // Permanantly disabling these, they frighten me in the field.
    bool disableInitialTransmissions = SelfRestart::didWeJustRestart() || !configuration.sendInitialTransmissions();



    if (disableInitialTransmissions) {
        initialWeatherTransmissionSent = !InitialTransmissions::alreadyDone(TRANSMISSION_TYPE_WEATHER);
        // We'll never have both.
        initialAtlasTransmissionSent = !InitialTransmissions::alreadyDone(TRANSMISSION_TYPE_ATLAS) || !InitialTransmissions::alreadyDone(TRANSMISSION_TYPE_SONAR);
        initialSonarTransmissionSent = !InitialTransmissions::alreadyDone(TRANSMISSION_TYPE_ATLAS) || !InitialTransmissions::alreadyDone(TRANSMISSION_TYPE_SONAR);
        initialLocationTransmissionSent = !InitialTransmissions::alreadyDone(TRANSMISSION_TYPE_LOCATION);
        DEBUG_PRINTLN("Initial transmission disabled.");
    }

    weatherStation.setup();

    memzero((uint8_t *)&location, sizeof(gps_location_t));

    DEBUG_PRINTLN("Loop");

    logPrinter.flush();

    #ifdef TESTING_MODE
    bool enabled = false;
    while (1) {
        if (Serial.available()) {
            while (Serial.available()) {
                char c = Serial.read();
                switch (c) {
                case 't': {
                    enabled = !enabled;
                    Serial.print("Toggled ");
                    Serial.println(enabled);

                    digitalWrite(PIN_ROCK_BLOCK, enabled);
                    break;
                }
                case 's': {
                    RockBlock rockBlock("TEST");
                    Serial1.begin(19200);
                    SerialType &rockBlockSerial = Serial1;
                    rockBlock.setSerial(&rockBlockSerial);
                    while (!rockBlock.isDone() && !rockBlock.isFailed()) {
                        rockBlock.tick();
                        delay(10);
                    }
                    break;
                }
                }
            }
        }

        delay(100);

        Watchdog.reset();
    }
    #endif
}

void checkWeatherStation() {
    Queue queue;

    Watchdog.reset();

    DEBUG_PRINTLN("WS: Check");
    logPrinter.flush();

    uint32_t started = millis();
    while (millis() - started < WEATHER_STATION_CHECK_TIME) {
        weatherStation.tick();

        Watchdog.reset();

        if (weatherStation.hasReading()) {
            DEBUG_PRINTLN("");

            DEBUG_PRINT("&");

            weatherStation.logReadingLocally();

            float *values = weatherStation.getValues();
            DEBUG_PRINT("%");
            if (SystemClock.set((uint32_t)values[FK_WEATHER_STATION_FIELD_UNIXTIME])) {
                // DEBUG_PRINTLN("Removing TransmissionStatus due to clock change.")
                // TransmissionStatus status;
                // status.remove();
            }

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

            queue.enqueue((uint8_t *)&packet, sizeof(weather_station_packet_t));

            weatherStation.clear();
            DEBUG_PRINTLN("^");
            logPrinter.flush();
        }

        delay(10);
    }

    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("WS: Done");
    logPrinter.flush();
}

void checkAirwaves() {
    Queue queue;
    LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST);
    NetworkProtocolState networkProtocol(NetworkState::EnqueueFromNetwork, &radio, &queue, new CollectorNetworkCallbacks());

    DEBUG_PRINTLN("AW: Check");
    logPrinter.flush();

    // Can't call this more than 3 times or so because we use up all the IRQs and
    // so this would be nice to have a kind of memory?
    if (!radioSetup) {
        if (radio.setup()) {
            radio.sleep();
        }
        else {
            logPrinter.flush();
            platformCatastrophe(PIN_RED_LED);
        }
        radioSetup = true;
    }

    DEBUG_PRINTLN("AW: RR");
    logPrinter.flush();

    uint32_t started = millis();
    uint32_t last = millis();
    while (millis() - started < AIRWAVES_CHECK_TIME || !networkProtocol.isQuiet()) {
        networkProtocol.tick();

        weatherStation.ignore();

        if (millis() - last > 5000) {
            platformBlink(PIN_RED_LED);
            Serial.print(".");
            last = millis();

            Watchdog.reset();
        }

        if (transmissionForced) {
            break;
        }

        delay(10);
    }

    radio.sleep();
    delay(100);

    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("AW: Done");
    logPrinter.flush();

    SelfRestart::restartIfNecessary();

    DEBUG_PRINTLN("AW: Exit");
    logPrinter.flush();
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

String sonarPacketToMessage(sonar_station_packet_t *packet) {
    String message(packet->time);
    message += ",";
    message += configuration.getName();
    message += ",";
    message += String(packet->battery, 2);
    for (uint8_t i = 0; i < FK_SONAR_STATION_PACKET_NUMBER_VALUES; ++i) {
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

    digitalWrite(PIN_RED_LED, HIGH);

    bool success = false;
    uint32_t started = millis();
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
                weatherStation.tick();
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
                weatherStation.tick(); // Remember with the IridiumSBD library
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
        numberOfFailures++;
    }

    digitalWrite(PIN_RED_LED, LOW);

    return success;
}

void handleSensorTransmission(bool triggered, bool sendAtlas, bool sendWeather, bool sendSonar) {
    Queue queue;

    uint32_t queueSize = queue.size();

    DEBUG_PRINT("TS: handleSensor: ");
    DEBUG_PRINT(queueSize);
    DEBUG_PRINT(" triggered=");
    DEBUG_PRINT(triggered);
    DEBUG_PRINT(" sendAtlas=");
    DEBUG_PRINT(sendAtlas);
    DEBUG_PRINT(" sendWeather=");
    DEBUG_PRINT(sendWeather);
    DEBUG_PRINT(" sendSonar=");
    DEBUG_PRINT(sendSonar);
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
            if (singleTransmission(atlasPacketToMessage(&atlas_station_sensors))) {
                initialAtlasTransmissionSent = true; // We'll never have both.
                initialSonarTransmissionSent = true;
                InitialTransmissions::markCompleted(TRANSMISSION_TYPE_ATLAS);
            }
        }
        else {
            noAtlas = true;
        }
    }

    if (sendSonar) {
        if (sonar_station_sensors.fk.kind == FK_PACKET_KIND_SONAR_STATION) {
            if (singleTransmission(sonarPacketToMessage(&sonar_station_sensors))) {
                initialSonarTransmissionSent = true; // We'll never have both.
                initialAtlasTransmissionSent = true;
                InitialTransmissions::markCompleted(TRANSMISSION_TYPE_SONAR);
            }
            else {
                noSonar = true;
            }
        }
    }

    if (sendWeather) {
        if (weather_station_sensors.fk.kind == FK_PACKET_KIND_WEATHER_STATION) {
            if (singleTransmission(weatherStationPacketToMessage(&weather_station_sensors))) {
                initialWeatherTransmissionSent = true;
                InitialTransmissions::markCompleted(TRANSMISSION_TYPE_WEATHER);
            }
        }
        else {
            noWeather = true;
        }
    }

    if (triggered) {
        if ((noAtlas && noSonar) || noWeather) {
            uint32_t uptime = millis() / (1000 * 60);
            String message(SystemClock.now());
            message += ",";
            message += configuration.getName();
            message += ",";
            message += noAtlas;
            message += ",";
            message += noSonar;
            message += ",";
            message += noWeather;
            message += ",";
            message += queueSize;
            message += ",";
            message += numberOfFailures;
            message += ",";
            message += uptime;
            singleTransmission(message);
        }
    }
}

String locationToMessage(gps_location_t *location) {
    uint32_t uptime = millis() / (1000 * 60);
    String message(location->time);
    message += ",";
    message += configuration.getName();
    message += "," + String(location->latitude, 6);
    message += "," + String(location->longitude, 6);
    message += "," + String(location->altitude, 2);
    message += ",";
    message += uptime;
    return message;
}

void handleLocationTransmission() {
    if (location.time > 0) {
        if (singleTransmission(locationToMessage(&location))) {
            initialLocationTransmissionSent = true;
            InitialTransmissions::markCompleted(TRANSMISSION_TYPE_WEATHER);
        }
    }
}

void handleTransmissionIfNecessary() {
    TransmissionStatus status;

    int8_t kind = status.shouldWe();
    if (kind == TRANSMISSION_KIND_SENSORS) {
        handleSensorTransmission(true, true, true, true);
    }
    else if (kind == TRANSMISSION_KIND_LOCATION) {
        handleLocationTransmission();
    }

    if (transmissionForced) {
        handleSensorTransmission(true, true, true, true);
        handleLocationTransmission();
        transmissionForced = false;
    }

    if (!initialAtlasTransmissionSent || !initialWeatherTransmissionSent || !initialSonarTransmissionSent) {
        handleSensorTransmission(false, !initialAtlasTransmissionSent, !initialWeatherTransmissionSent, !initialSonarTransmissionSent);
    }
    if (!initialLocationTransmissionSent) {
        handleLocationTransmission();
    }
}

void idlePeriod() {
    DEBUG_PRINTLN("Idle: Begin");
    logPrinter.flush();

    uint32_t started = millis();
    while (millis() - started < IDLE_PERIOD) {
        Watchdog.reset();
        delay(2000);
        platformBlinks(PIN_RED_LED, 2);
    }

    DEBUG_PRINTLN("Idle: Done");
    logPrinter.flush();
}

enum class CollectorState {
    Airwaves,
    WeatherStation,
    Idle,
    Transmission
};

void logTransition(const char *name) {
    uint32_t easternTime = 4 * 60 * 60;
    DateTime dt(SystemClock.now() - easternTime);

    DEBUG_PRINT("## ");

    DEBUG_PRINT(dt.unixtime());

    DEBUG_PRINT(' ');
    DEBUG_PRINT(dt.year());
    DEBUG_PRINT('/');
    DEBUG_PRINT(dt.month());
    DEBUG_PRINT('/');
    DEBUG_PRINT(dt.day());
    DEBUG_PRINT(' ');
    DEBUG_PRINT(dt.hour());
    DEBUG_PRINT(':');
    DEBUG_PRINT(dt.minute());
    DEBUG_PRINT(':');
    DEBUG_PRINT(dt.second());

    DEBUG_PRINT(" ");
    DEBUG_PRINT(platformBatteryVoltage());

    DEBUG_PRINT(" >");
    DEBUG_PRINTLN(name);
}

void loop() {
    CollectorState state = CollectorState::Airwaves;

    while (true) {
        switch (state) {
        case CollectorState::Airwaves: {
            checkAirwaves();
            logTransition("WS");
            state = CollectorState::WeatherStation;
            break;
        }
        case CollectorState::WeatherStation: {
            checkWeatherStation();
            logTransition("ID");
            state = CollectorState::Idle;
            break;
        }
        case CollectorState::Idle: {
            idlePeriod();
            logTransition("TX");
            state = CollectorState::Transmission;
            break;
        }
        case CollectorState::Transmission: {
            handleTransmissionIfNecessary();
            logTransition("AW");
            state = CollectorState::Airwaves;
            break;
        }
        }
        UptimeTracker::remember();
    }
}

// vim: set ft=cpp:
