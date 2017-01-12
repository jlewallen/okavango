#include <SoftwareSerial.h>
#include <Wire.h>
#include <SparkFunMPL3115A2.h>
#include <SparkFunHTU21D.h>
#include <TinyGPS++.h>
#include <RTClib.h>

MPL3115A2 myPressure;
HTU21D myHumidity;
SoftwareSerial gpsSerial(5, 4);
TinyGPSPlus gps;

const byte PIN_IRQ_WIND_SPEED = 3;
const byte PIN_IRQ_RAIN = 2;
const byte PIN_LED_BLUE = 7;
const byte PIN_LED_RED = 8;
const byte PIN_GPS_POWER = 6;

const byte PIN_REFERENCE_3V3 = A3;
const byte PIN_LIGHT = A1;
const byte PIN_WIND_DIRECTION = A0;

long lastSecond;  // The millis counter to see when a second rolls by
byte seconds;     // When it hits 60, increase the current minute
byte seconds_2m;  // Keeps track of the "wind speed/dir avg" over last 2 minutes array of data
byte minutes;     // Keeps track of where we are in various arrays of data
byte minutes_10m; // Keeps track of where we are in wind gust/dir over last 10 minutes array of data

long lastWindCheck = 0;
volatile long lastWindIRQ = 0;
volatile byte windClicks = 0;

// We need to keep track of the following variables:
// Wind speed/dir each update (no storage)
// Wind gust/dir over the day (no storage)
// Wind speed/dir, avg over 2 minutes (store 1 per second)
// Wind gust/dir over last 10 minutes (store 1 per minute)
// Rain over the past hour (store 1 per minute)
// Total rain over date (store one per day)

byte windspdavg[120]; // 120 bytes to keep track of 2 minute average

#define WIND_DIR_AVG_SIZE 120
int winddiravg[WIND_DIR_AVG_SIZE]; // 120 ints to keep track of 2 minute average
float windgust_10m[10];            // 10 floats to keep track of 10 minute max
int windgustdirection_10m[10];     // 10 ints to keep track of 10 minute max
volatile float rainHour[60];       // 60 floating numbers to keep track of 60 minutes of rain

// These are all the weather values that wunderground expects:
int winddir = 0; // [0-360 instantaneous wind direction]
float windspeedmph = 0; // [mph instantaneous wind speed]
float windgustmph = 0; // [mph current wind gust, using software specific time period]
int windgustdir = 0; // [0-360 using software specific time period]
float windspdmph_avg2m = 0; // [mph 2 minute average wind speed mph]
int winddir_avg2m = 0; // [0-360 2 minute average wind direction]
float windgustmph_10m = 0; // [mph past 10 minutes wind gust mph ]
int windgustdir_10m = 0; // [0-360 past 10 minutes wind gust direction]
float humidity = 0; // [%]
float temperature = 0; // [temperature F]
float rainin = 0; // [rain inches over the past hour)] -- the accumulated rainfall in the past 60 min
volatile float dailyrainin = 0; // [rain inches so far today in local time]
float pressure = 0;
float light_lvl = 455;

volatile unsigned long raintime, rainlast, raininterval, rain;

extern float get_wind_speed();
extern uint32_t get_wind_direction();
extern float get_light_level();
extern void print_weather();
extern void calculate_weather();
extern void delay_and_check_gps(uint32_t ms);

// Activated by the magnet and reed switch in the rain gauge.
void irq_handler_rain() {
    raintime = millis();
    raininterval = raintime - rainlast;

    // Debounce
    if (raininterval > 10) {
        dailyrainin += 0.011;       // Each dump is 0.011" of water
        rainHour[minutes] += 0.011; // Increase this minute's amount of rain
        rainlast = raintime;
    }
}

// Activated by the magnet in the anemometer (2 ticks per rotation), attached to input D3
void irq_handler_wind_speed() {
    // Ignore switch-bounce glitches less than 10ms (142MPH max reading) after the reed switch closes
    if (millis() - lastWindIRQ > 10) {
        lastWindIRQ = millis();
        windClicks++; // There is 1.492MPH for each click per second.
    }
}

void setup() {
    Serial.begin(9600);

    gpsSerial.begin(9600);

    pinMode(PIN_LED_BLUE, OUTPUT);
    pinMode(PIN_LED_RED, OUTPUT);

    pinMode(PIN_GPS_POWER, OUTPUT);
    digitalWrite(PIN_GPS_POWER, HIGH); // Pulling this pin low puts GPS to sleep but maintains RTC and RAM

    pinMode(PIN_IRQ_WIND_SPEED, INPUT_PULLUP);
    pinMode(PIN_IRQ_RAIN, INPUT_PULLUP);

    pinMode(PIN_REFERENCE_3V3, INPUT);
    pinMode(PIN_LIGHT, INPUT);

    myPressure.begin();
    myPressure.setModeBarometer();   // Measure pressure in Pascals from 20 to 110 kPa
    myPressure.setOversampleRate(7); // Set Oversample to the recommended 128
    myPressure.enableEventFlags();   // Enable all three pressure and temp event flags

    myHumidity.begin();

    seconds = 0;
    lastSecond = millis();

    // Attach external interrupt pins to IRQ functions
    attachInterrupt(0, irq_handler_rain, FALLING);
    attachInterrupt(1, irq_handler_wind_speed, FALLING);

    interrupts();
}

void loop() {
    if (millis() - lastSecond >= 1000) {
        int led = gps.satellites.value() > 0 ? PIN_LED_BLUE : PIN_LED_RED;
        digitalWrite(led, HIGH);

        lastSecond += 1000;

        // Take a speed and direction reading every second for 2 minute average
        if (++seconds_2m > 119) seconds_2m = 0;

        // Calc the wind speed and direction every second for 120 second to get 2 minute average
        float currentSpeed = get_wind_speed();
        int32_t currentDirection = get_wind_direction();
        windspeedmph = currentSpeed;
        windspdavg[seconds_2m] = (int32_t)currentSpeed;
        winddiravg[seconds_2m] = currentDirection;

        // Check to see if this is a gust for the minute
        if (currentSpeed > windgust_10m[minutes_10m]) {
            windgust_10m[minutes_10m] = currentSpeed;
            windgustdirection_10m[minutes_10m] = currentDirection;
        }

        // Check to see if this is a gust for the day
        if (currentSpeed > windgustmph) {
            windgustmph = currentSpeed;
            windgustdir = currentDirection;
        }

        if (++seconds > 59) {
            seconds = 0;

            if (++minutes > 59) minutes = 0;
            if (++minutes_10m > 9) minutes_10m = 0;

            rainHour[minutes] = 0; // Zero out this minute's rainfall amount
            windgust_10m[minutes_10m] = 0; // Zero out this minute's gust
        }

        print_weather();

        digitalWrite(led, LOW);
    }

    delay_and_check_gps(800);
}

// While we delay for a given amount of time, gather GPS data
void delay_and_check_gps(uint32_t ms) {
    uint32_t start = millis();
    do {
        while (gpsSerial.available()) {
            gps.encode(gpsSerial.read());
        }
    }
    while (millis() - start < ms);
}

void calculate_weather() {
    winddir = get_wind_direction();

    float temp = 0;
    for (int32_t i = 0 ; i < 120 ; i++) {
        temp += windspdavg[i];
    }
    temp /= 120.0;
    windspdmph_avg2m = temp;

    // Calc winddir_avg2m, Wind Direction
    // You can't just take the average. Google "mean of circular quantities" for more info
    // We will use the Mitsuta method because it no trig functions
    // Based on: http://abelian.org/vlf/bearings.html
    // Based on: http://stackoverflow.com/questions/1813483/averaging-angles-again
    long sum = winddiravg[0];
    int32_t D = winddiravg[0];
    for (int32_t i = 1 ; i < WIND_DIR_AVG_SIZE ; i++) {
        int32_t delta = winddiravg[i] - D;

        if (delta < -180) {
            D += delta + 360;
        }
        else if(delta > 180) {
            D += delta - 360;
        }
        else {
            D += delta;
        }

        sum += D;
    }
    winddir_avg2m = sum / WIND_DIR_AVG_SIZE;
    if (winddir_avg2m >= 360) winddir_avg2m -= 360;
    if (winddir_avg2m < 0) winddir_avg2m += 360;

    windgustmph_10m = 0;
    windgustdir_10m = 0;
    for (int32_t i = 0; i < 10 ; i++) {
        if (windgust_10m[i] > windgustmph_10m) {
            windgustmph_10m = windgust_10m[i];
            windgustdir_10m = windgustdirection_10m[i];
        }
    }

    humidity = myHumidity.readHumidity();

    temperature = myPressure.readTemp();

    rainin = 0;

    for (int32_t i = 0 ; i < 60 ; i++) {
        rainin += rainHour[i];
    }

    pressure = myPressure.readPressure();

    light_lvl = get_light_level();
}

float get_light_level() {
    float operatingVoltage = analogRead(PIN_REFERENCE_3V3);
    float lightSensor = analogRead(PIN_LIGHT);

    operatingVoltage = 3.3 / operatingVoltage; // the reference voltage is 3.3V

    lightSensor = operatingVoltage * lightSensor;

    return lightSensor;
}

float get_wind_speed() {
    float deltaTime = millis() - lastWindCheck; // 750ms

    deltaTime /= 1000.0;

    float windSpeed = (float)windClicks / deltaTime; // 3 / 0.750s = 4

    windClicks = 0;
    lastWindCheck = millis();

    windSpeed *= 1.492; // 4 * 1.492 = 5.968MPH

    return windSpeed;
}

uint32_t get_wind_direction() {
    uint32_t adc = analogRead(PIN_WIND_DIRECTION);

    // The following table is ADC readings for the wind direction sensor output, sorted from low to high.
    // Each threshold is the midpoint between adjacent headings. The output is degrees for that ADC reading.
    // Note that these are not in compass degree order! See Weather Meters datasheet for more information.

    if (adc < 380) return 113;
    if (adc < 393) return 68;
    if (adc < 414) return 90;
    if (adc < 456) return 158;
    if (adc < 508) return 135;
    if (adc < 551) return 203;
    if (adc < 615) return 180;
    if (adc < 680) return 23;
    if (adc < 746) return 45;
    if (adc < 801) return 248;
    if (adc < 833) return 225;
    if (adc < 878) return 338;
    if (adc < 913) return 0;
    if (adc < 940) return 293;
    if (adc < 967) return 315;
    if (adc < 990) return 270;
    return -1;
}

#define PRINT_LABEL(label)                  Serial.print(",")
#define PRINT_VALUE_FLT(value, decimals)    Serial.print(value, decimals)
#define PRINT_VALUE_INT(value)              Serial.print(value)

void print_weather() {
    calculate_weather();

    DateTime now = DateTime(gps.date.year(), gps.date.month(), gps.date.day(), gps.time.hour(), gps.time.minute(), gps.time.second());
    if (gps.satellites.value() > 0) {
        PRINT_VALUE_INT(now.unixtime());
    }
    else {
        PRINT_VALUE_INT(0);
    }

    PRINT_LABEL("winddir=");
    PRINT_VALUE_INT(winddir);
    PRINT_LABEL(",windspeedmph=");
    PRINT_VALUE_FLT(windspeedmph, 1);
    PRINT_LABEL(",windgustmph=");
    PRINT_VALUE_FLT(windgustmph, 1);
    PRINT_LABEL(",windgustdir=");
    PRINT_VALUE_INT(windgustdir);
    PRINT_LABEL(",windspdmph_avg2m=");
    PRINT_VALUE_FLT(windspdmph_avg2m, 1);
    PRINT_LABEL(",winddir_avg2m=");
    PRINT_VALUE_FLT(winddir_avg2m, 1);
    PRINT_LABEL(",windgustmph_10m=");
    PRINT_VALUE_FLT(windgustmph_10m, 1);
    PRINT_LABEL(",windgustdir_10m=");
    PRINT_VALUE_INT(windgustdir_10m);
    PRINT_LABEL(",humidity=");
    PRINT_VALUE_FLT(humidity, 1);
    PRINT_LABEL(",temp=");
    PRINT_VALUE_FLT(temperature, 1);
    PRINT_LABEL(",rainin=");
    PRINT_VALUE_FLT(rainin, 2);
    PRINT_LABEL(",dailyrainin=");
    PRINT_VALUE_FLT(dailyrainin, 2);
    PRINT_LABEL(",pressure=");
    PRINT_VALUE_FLT(pressure, 2);
    PRINT_LABEL(",light_lvl=");
    PRINT_VALUE_FLT(light_lvl, 2);

    PRINT_LABEL(",lat=");
    PRINT_VALUE_FLT(gps.location.lat(), 6);
    PRINT_LABEL(",lat=");
    PRINT_VALUE_FLT(gps.location.lng(), 6);
    PRINT_LABEL(",altitude=");
    PRINT_VALUE_INT(gps.altitude.meters());
    PRINT_LABEL(",sats=");
    PRINT_VALUE_INT(gps.satellites.value());

    PRINT_LABEL(",date=");
    PRINT_VALUE_INT(gps.date.value());
    PRINT_LABEL(",time=");
    PRINT_VALUE_INT(gps.time.value());

    Serial.println();
}

// vim: set ft=cpp:
