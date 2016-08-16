#include "WeatherStation.h"
#include "protocol.h"
#include "Logger.h"

#ifdef ARDUINO_SAMD_FEATHER_M0

WeatherStation::WeatherStation() {
    clear();
}

void WeatherStation::setup() {
    hup();

    platformSerial2Begin(9600);
}

void WeatherStation::clear() {
    numberOfValues = 0;
    buffer[0] = 0;
    length = 0;
}

void WeatherStation::ignore() {
    while (Serial2.available()) {
        Serial2.read();
    }
}

void WeatherStation::off() {
    Serial2.end();

    pinMode(PIN_WEATHER_STATION_RESET, OUTPUT);
    digitalWrite(PIN_WEATHER_STATION_RESET, LOW);
}

void WeatherStation::hup() {
    pinMode(PIN_WEATHER_STATION_RESET, OUTPUT);
    digitalWrite(PIN_WEATHER_STATION_RESET, LOW);
    delay(500);
    digitalWrite(PIN_WEATHER_STATION_RESET, HIGH);
    delay(1000);
}

bool WeatherStation::tick() {
    if (Serial2.available()) {
        delay(50);

        while (Serial2.available()) {
            int16_t c = Serial2.read();
            if (c >= 0) {
                Serial.print((char)c);
                if (c == ',' || c == '\r' || c == '\n') {
                    if (length > 0) {
                        buffer[length] = 0;
                        if (numberOfValues < FK_WEATHER_STATION_MAX_VALUES) {
                            values[numberOfValues++] = atof(buffer);
                        }
                        length = 0;;
                    }
                    if (c == '\r' || c == '\n') {
                        bool success = numberOfValues == FK_WEATHER_STATION_PACKET_NUMBER_VALUES;
                        numberOfValues = 0;
                        return success;
                    }
                }
                else if (length < FK_WEATHER_STATION_MAX_BUFFER - 1) {
                    buffer[length++] = (char)c;
                    buffer[length] = 0;
                }
            }
        }
    }

    return false;
}

void WeatherStation::logReadingLocally() {
    File file = Logger::open(FK_SETTINGS_WEATHER_STATION_DATA_FILENAME);
    if (file) {
        Serial.print("*");
        for (uint8_t i = 0; i < numberOfValues; ++i) {
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
