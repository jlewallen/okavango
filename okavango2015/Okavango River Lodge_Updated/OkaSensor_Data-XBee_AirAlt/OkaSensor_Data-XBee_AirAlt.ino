#include <XBee.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <SoftwareSerial.h>

#define XBEE_DESTINATION_HIGH 0x0013A200
#define XBEE_DESTINATION_LOW 0x40B7AAC4
#include "XBeeUtilities.h"

#define ONE_WIRE_BUS     6 // Water temp wire is plugged into pin 2 on the Arduino
#define DHTPIN           7 // what pin we're connected to
#define DHTTYPE          DHT22 // set type of sensor to DHT 22  (AM2302)
#define SD_BOARD_PIN     10

// Setup a oneWire instance to communicate with any OneWire devices 
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
// Initialize DHT sensor for normal 16mhz Arduino
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);
RTC_DS1307 RTC; // define the Real Time Clock object
File logfile;
float h, t, f;

void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);
  while(1);
}

void openLogFile()
{
  pinMode(SD_BOARD_PIN, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(SD_BOARD_PIN)) {
    error("Card failed");
  }
  
  // create a new file
  for (long i = 0; i <= 99999999; i++) {
    char filename[13];
    String fn(i);
    while (fn.length() < 8) {
      fn = '0' + fn;
    }
    fn = fn + ".CSV";
    fn.toCharArray(filename, sizeof(filename));
    if (!SD.exists(filename)) {
      // only open a new file if it doesn't exist
      Serial.print("File: ");
      Serial.println(filename);
      logfile = SD.open(filename, FILE_WRITE); 
      break;
    }
  }

  if (!logfile) {
    error("EFILE");
  }
  logfile.flush();
}

void setup(){             
    Serial.begin(115200);      
    xbee.setSerial(xbeeSerial);

    Wire.begin();    
    RTC.begin();
    
    sensors.begin();             // Start up the library for water sensor
    dht.begin();                 // Start up the library for air temp/humidity sensor
    bmp.begin();      // Start up the library for the altitude sensor

    openLogFile();

    configureSleepMode();
}

void loopWaterTemperature()
{
    // call sensors.requestTemperatures() to issue a global temperature
    // request to all devices on the bus
    sensors.requestTemperatures(); // Send the command to get temperatures
}

void loopAirTemperatureAndHumidity()
{
     // Wait a few seconds between measurements.
    delay(2000); 

    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    h = dht.readHumidity();
    // Read temperature as Celsius
    t = dht.readTemperature();
    // Read temperature as Fahrenheit
    f = dht.readTemperature(true);
  
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("9999");
      return;
    }
}

sensors_event_t event;
  
float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;

void loopAltitude()
{
  bmp.getEvent(&event);
  float temperature;
  bmp.getTemperature(&temperature);
}

void loop(){                   
  Serial.println("Loop");

  logfile.println("Loop");
  logfile.flush();

  DateTime now = RTC.now();
  loopAirTemperatureAndHumidity();
  loopAltitude();

  /////////////////////////////////////////////////////////////////////
  //  Send all data to serial output 
  /////////////////////////////////////////////////////////////////////
  logfile.print(now.unixtime());
  logfile.print(",");
  logfile.print(h);
  logfile.print(",");
  logfile.print(t);
  logfile.print(",");
  logfile.print(event.pressure);
  logfile.print(",");
  logfile.print("\n");

  Serial.print(now.unixtime());
  Serial.print(",");
  Serial.print(h);
  Serial.print(",");
  Serial.print(t);
  Serial.print(",");
  Serial.print(event.pressure);
  Serial.print(",");
  Serial.print("\n");
  
  logfile.flush();

  memset((void *)&payload, 0, sizeof(payload));
  payload.kind = 3;
  payload.v1 = h;
  payload.v2 = t;
  payload.v3 = event.pressure; /* kPa to hPa */
  payload.time = now.unixtime();
  longDelayAndAttemptToSendPacket(60L * 1000L * 60L * 6L);
}
