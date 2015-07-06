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

#define DHTPIN           3 // what pin we're connected to
#define COND_RXPIN       4 // set the conductivity (EC sensor) RX pin (labeled "TX" on EC board)
#define COND_TXPIN       5 // set the conductivity (EC sensor) TX pin (labeled "RX" on EC board)
#define ONE_WIRE_BUS     6 // Water temp wire is plugged into pin 2 on the Arduino
#define cellRX           7
#define cellTX           8
#define DHTTYPE          DHT22 // set type of sensor to DHT 22  (AM2302)
#define SD_BOARD_PIN     10

float tdsValue;
float salinityValue;

SoftwareSerial gprsSerial(cellRX, cellTX);
SoftwareSerial cond(COND_RXPIN, COND_TXPIN);   
// Setup a oneWire instance to communicate with any OneWire devices 
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
DHT dht(DHTPIN, DHTTYPE);
RTC_DS1307 RTC;
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

void setup() {
  Serial.begin(115200);   
  gprsSerial.begin(19200);      // the GPRS baud rate    
  
  Wire.begin();
  RTC.begin();

  sensors.begin();             // Start up the library for water sensor
  dht.begin();                 // Start up the library for air temp/humidity sensor

  openLogFile();  
}

///SendTextMessage()
///this function is to send a sms message
void SendTextMessage(String message)
{
  gprsSerial.print("AT+CMGF=1\r");    //Because we want to send the SMS in text mode
  delay(100);
  gprsSerial.println("AT + CMGS = \"+13473940024\"");//send sms message, be careful need to add a country code before the cellphone number
  delay(100);
  gprsSerial.println(message);//the content of the message
  delay(100);
  gprsSerial.println((char)26);//the ASCII code of the ctrl+z is 26
  delay(100);
  gprsSerial.println();
}

void loopConductivity()
{
  String rawSensor = "";               
  boolean gotEnd = false; 

   cond.begin(9600);          // baud rate for cond sensor = 9600 (known)
   delay(1000);               // allow for sensor to start communicating
   cond.print("r \r");        // send string "r" with a carriage return "\r" to take one reading
   delay(1000);               // wait for reading to be read and sent
   
   while (cond.available() > 0) {
      char c = (char)cond.read();
      rawSensor += c;
      if (c == '\r') {
        gotEnd = true;
      }
      delay(50);
   }            
   cond.end();   

   if (gotEnd) {
      int delim1 = rawSensor.indexOf(',');
      int delim2 = rawSensor.indexOf(',', delim1 + 1);
      int delim3 = rawSensor.indexOf('*', delim2 + 1);
      String tdsString = rawSensor.substring(delim1, delim2 - delim1);
      String salinityString = rawSensor.substring(delim2, delim3 - delim2);
      tdsValue = tdsString.toFloat();
      salinityValue = salinityString.toFloat();
   }   
   else {                                           // if no variables are found within the pickup call = set variable to 9999
      tdsValue = 9999;
      salinityValue = 9999;
   }   
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
  loopConductivity();
  loopWaterTemperature();
  loopAirTemperatureAndHumidity();

  /////////////////////////////////////////////////////////////////////
  //  Send all data to serial output 
  /////////////////////////////////////////////////////////////////////
  logfile.print(now.unixtime());
  logfile.print(",");
  logfile.print(tdsValue);
  logfile.print(",");
  logfile.print(salinityValue);
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
  Serial.print(tdsValue);
  Serial.print(",");
  Serial.print(salinityValue);
  Serial.print(",");
  Serial.print(sensors.getTempCByIndex(0)); 
  Serial.print(",");
  Serial.print(h);
  Serial.print(",");
  Serial.print(t);
  Serial.print(",");
  Serial.print("\n");
  
  logfile.flush();

/* Need to figure out how to build the message to be "Time,Loc,TDS,Sal,Wat,Air,Hum" within 140 char */
  String textForSMS = "";  
  textForSMS += now.unixtime();
  textForSMS += ",";
  textForSMS += "Loc";
  textForSMS += ",";
  textForSMS += String(tdsValue, 3);
  textForSMS += ",";
  textForSMS += String(salinityValue, 3);
  textForSMS += ",";
  textForSMS += String(sensors.getTempCByIndex(0), 3);
  textForSMS += ",";
  textForSMS += String(t, 3);
  textForSMS += ",";
  textForSMS += String(h, 3);
  
 /* not quite sure how to add in the rest of the data */
  Serial.println("Message:");
  Serial.println(textForSMS);
  SendTextMessage(textForSMS);
  delay(1000);
  Serial.println("Done waiting"); 

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
      if (++i == 2 * 20) {
        break;
      }
    }
    delay(5000);
  }
  Serial.println("Done");
}
