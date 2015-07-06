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
#define PH_RXPIN         4 // set the pH sensor RX pin (labeled "TX" on pH board)
#define PH_TXPIN         5 // set the pH sensor TX pin (labeled "RX" on pH board)
#define ONE_WIRE_BUS     6 // Water temp wire is plugged into pin 6 on the Arduino
#define cellRX           7
#define cellTX           8
#define DHTTYPE          DHT22 // set type of sensor to DHT 22  (AM2302)
#define SD_BOARD_PIN     10

String textForSMS;
float phValue;

SoftwareSerial gprsSerial(cellRX, cellTX);
SoftwareSerial pH(PH_RXPIN, PH_TXPIN);
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

String AppendFloat(String oldString, float num)
{
  char buf[20];
  char* string = dtostrf(num, 4, 2, buf);
  oldString.concat(string);
        return oldString;
}

void loopPh()
{
   String rawSensor = "";               
   boolean gotEnd = false; 

   // same code to run conductivity above except pH only outputs 1 number vs 3 for conductivity sensor
   pH.begin(9600);           // baud rate for pH sensor = 9600 (known)
   delay(1000);
   pH.print("r \r");          // send string "r" with a carriage return "\r" to take one reading
   delay(3000);               // it takes a while for the pH sensor to respond

   while (pH.available() > 0) {                     // message is being sent              
      char c = (char)pH.read();             // string msg is read as individual characters       
      rawSensor += c;                              
      if (c == '\r') {
        gotEnd = true;
       }
      delay(50);
   }         
   pH.end();
   
   if (gotEnd){
      int n = rawSensor.indexOf('*');
      String phString = rawSensor.substring(0, n);
      phValue = phString.toFloat();
   }
   else{
      phValue = 9999;
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
  loopPh();
  loopWaterTemperature();
  loopAirTemperatureAndHumidity();

  /////////////////////////////////////////////////////////////////////
  //  Send all data to serial output 
  /////////////////////////////////////////////////////////////////////
  logfile.print(now.unixtime());
  logfile.print(",");
  logfile.print(phValue);
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
  Serial.print(phValue);
  Serial.print(",");
  Serial.print(sensors.getTempCByIndex(0)); 
  Serial.print(",");
  Serial.print(h);
  Serial.print(",");
  Serial.print(t);
  Serial.print(",");
  Serial.print("\n");
  
  logfile.flush();

/* Need to figure out how to build the message to be "Time,Loc,pH,Wat,Air,Hum" within 140 char */
  textForSMS = "Time";  
  textForSMS = AppendFloat(textForSMS, now.unixtime());
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
