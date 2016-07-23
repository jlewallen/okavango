#include <XBee.h>
#include <SD.h>
#include <SPI.h>
#include <RTClib.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MPL115A2.h>
#include <SoftwareSerial.h>

#define XBEE_DESTINATION_HIGH 0x0013A200
#define XBEE_DESTINATION_LOW 0x409F2937
#include "XBeeUtilities.h"

#define DHTPIN           7 // what pin we're connected to
#define DHTTYPE          DHT22 // set type of sensor to DHT 22  (AM2302)
#define SD_BOARD_PIN     10

// Initialize DHT sensor for normal 16mhz Arduino
DHT dht(DHTPIN, DHTTYPE);
Adafruit_MPL115A2 mpl115a2;
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
    error("Card failed, or not present");
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
      Serial.print("Logging to: ");
      Serial.println(filename);
      logfile = SD.open(filename, FILE_WRITE); 
      break;
    }
  }

  if (!logfile) {
    error("couldnt create file");
  }
  logfile.flush();
}

void setup(){             
    Serial.begin(115200);      
    xbee.setSerial(xbeeSerial);

    Wire.begin();    
    RTC.begin();
    dht.begin();                 // Start up the library for air temp/humidity sensor
    mpl115a2.begin();

    openLogFile();

    configureSleepMode();
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

void loop(){                   
  Serial.println("Starting new loop...");

  logfile.println("Starting new loop...");
  logfile.flush();

  DateTime now = RTC.now();
  loopAirTemperatureAndHumidity();
  float pressure = mpl115a2.getPressure(); 

  /////////////////////////////////////////////////////////////////////
  //  Send all data to serial output 
  /////////////////////////////////////////////////////////////////////
  logfile.print(now.unixtime());
  logfile.print(",");
  logfile.print(h);
  logfile.print(",");
  logfile.print(t);
  logfile.print(",");
  logfile.print(pressure);
  logfile.print("\n");

  Serial.print(now.unixtime());
  Serial.print(",");
  Serial.print(h);
  Serial.print(",");
  Serial.print(t);
  Serial.print(",");
  Serial.print(pressure);
  Serial.print("\n");
  
  logfile.flush();

  memset((void *)&payload, 0, sizeof(payload));
  payload.kind = 4;
  payload.time = now.unixtime();
  payload.v1 = h;
  payload.v2 = t;
  payload.v3 = pressure * 10; /* kPa to hPa */
  longDelayAndAttemptToSendPacket(60L * 1000L * 60L * 6L);
  
  Serial.println("Done");
}
