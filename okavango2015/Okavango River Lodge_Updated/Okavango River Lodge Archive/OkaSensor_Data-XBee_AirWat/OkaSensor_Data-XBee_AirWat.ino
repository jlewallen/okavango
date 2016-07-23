#include <XBee.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include "RTClib.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <SoftwareSerial.h>

#define ONE_WIRE_BUS     6 // Water temp wire is plugged into pin 2 on the Arduino
#define DHTPIN           7 // what pin we're connected to
#define DHTTYPE          DHT22 // set type of sensor to DHT 22  (AM2302)
#define SD_BOARD_PIN     10

SoftwareSerial xbeeSerial(2, 3); 
// Setup a oneWire instance to communicate with any OneWire devices 
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
DHT dht(DHTPIN, DHTTYPE);
RTC_DS1307 RTC;
File logfile;
XBee xbee = XBee();

#pragma pack(push, 1)

typedef struct {
  float v1;
  float v2;
  float v3;
  float v4;
  unsigned long time;
  char kind;
} packet_t;

packet_t payload;

#pragma pack(push, 4)

float h, t, f;

// SH + SL Address of receiving XBee (Okavango River Lodge)
XBeeAddress64 addr64 = XBeeAddress64(0x0013A200, 0x40B7AAC4); //this needs to be updated for the correct XBee Test configuration: 13A200  40C6746A
ZBTxRequest zbTx = ZBTxRequest(addr64, (uint8_t *)&payload, sizeof(payload));
ZBTxStatusResponse txStatus = ZBTxStatusResponse();

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

void setup() {
  xbeeSerial.begin(9600);  
  Serial.begin(115200);      
  xbee.setSerial(xbeeSerial);
  
  Wire.begin();
  RTC.begin();

  sensors.begin();             // Start up the library for water sensor
  dht.begin();                 // Start up the library for air temp/humidity sensor

  openLogFile();  
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

void loop(){                   
  Serial.println("Starting new loop...");

  logfile.println("Starting new loop...");
  logfile.flush();

  DateTime now = RTC.now();
  loopWaterTemperature();
  loopAirTemperatureAndHumidity();

  /////////////////////////////////////////////////////////////////////
  //  Send all data to serial output 
  /////////////////////////////////////////////////////////////////////
  logfile.print(now.unixtime());
  logfile.print(",");
  logfile.print(sensors.getTempCByIndex(0)); 
  logfile.print(",");
  logfile.print(h);
  logfile.print(",");
  logfile.print(t);
  logfile.print(",");
  logfile.print("\n");

  Serial.print(now.unixtime());
  Serial.print(",");
  Serial.print(sensors.getTempCByIndex(0)); 
  Serial.print(",");
  Serial.print(h);
  Serial.print(",");
  Serial.print(t);
  Serial.print(",");
  Serial.print("\n");
  
  logfile.flush();

  memset((void *)&payload, 0, sizeof(payload));
  payload.kind = 2;
  payload.v1 = sensors.getTempCByIndex(0);
  payload.v2 = h;
  payload.v3 = t;
  payload.time = now.unixtime();  
  xbee.send(zbTx);

  /* Delay for 5 minutes. Watch out for millis() wrapping around and just call that 
  the end of the delay. This doesn't always need to be 5mins exactly. */
  unsigned long startMillis = millis();
  unsigned long lastUpdate = startMillis;
  unsigned long i = 0;
  while (true) {
    unsigned long now = millis();
    unsigned long elapsed = now - startMillis;
    if (now - lastUpdate >= 30 * 1000) {
      Serial.println(elapsed);
      lastUpdate = now;
      if (++i == 2 * 5) {
        break;
      }
    }
    delay(5000);
  }
  Serial.println("Done");
}
