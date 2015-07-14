#include <XBee.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_Sensor.h>
#include <SoftwareSerial.h>

#define XBEE_DESTINATION_HIGH 0x0013A200
#define XBEE_DESTINATION_LOW 0x40B7AAC4
#include "XBeeUtilities.h"

#define COND_RXPIN       4 // set the conductivity (EC sensor) RX pin (labeled "TX" on EC board)
#define COND_TXPIN       5 // set the conductivity (EC sensor) TX pin (labeled "RX" on EC board)
#define ORP_RXPIN        6 // set the ORP RX pin (labeled "TX" on ORP board) - NEED TO CHANGE THIS FOR THE SD SHIELD
#define ORP_TXPIN        7 // set the ORP TX pin (labeled "RX" on ORP board) - NEED TO CHANGE THIS FOR THE SD SHIELD
#define SD_BOARD_PIN     10

// Create an instance of the softwareSerial class for each sensor
SoftwareSerial cond(COND_RXPIN, COND_TXPIN);  
SoftwareSerial orp(ORP_RXPIN, ORP_TXPIN);
SoftwareSerial xbeeSerial(2, 3); 
RTC_DS1307 RTC;
File logfile;
float tdsValue;
float salinityValue;
float orpValue;

void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);
  delay(1000L * 60L);
  resetArduino();
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

  if (logfile) {
    logfile.flush();
  }
  else {
    Serial.println("No log file.");
  }
}

void setup(){             
  xbeeSerial.begin(9600);  
  Serial.begin(115200);      
  xbee.setSerial(xbeeSerial);
  
  Wire.begin();
  RTC.begin();
  
  openLogFile();

  configureSleepMode();
}

void echo(SoftwareSerial &serial, uint32_t delay)
{
  uint32_t started = millis();
  while (millis() - started < delay) {
    int c = serial.read();
    if (c >= 0) {
      Serial.print((char)c);
    }
  }
}

void loopConductivity()
{
  String rawSensor = "";               
  boolean gotEnd = false; 

   cond.begin(9600);          // baud rate for cond sensor = 9600 (known)
   echo(cond, 1000);
   
   Serial.println("Configuring 1/2");
   cond.print("c,0\r");  
   echo(cond, 1000);
   cond.print("c,0\r");  
   echo(cond, 1000);
   
   Serial.println("Configuring 2/2");
   cond.print("K,0.1 \r");  
   echo(cond, 1000);
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

void loopOrp() 
{
   orp.begin(9600);           // baud rate for sensor = 9600 (known)
   delay(1000);
   orp.print("C,0 \r");   
   delay(1000);
   orp.print("r \r");         // send string "r" with a carriage return "\r" to take one reading
   delay(3000);               // it takes a while for the sensor to respond

   String rawSensor = "";               
   boolean gotEnd = false; 

   while (orp.available() > 0) {     
      char c = (char)orp.read();                 
      rawSensor += c;                              
      if (c == '\r') {
        gotEnd = true;
      }
      delay(50);
   }         
   orp.end();
   
   if (gotEnd){
      int n = rawSensor.indexOf('*'); // Find end 
      String orpString = rawSensor.substring(0, n);
      orpValue = orpString.toFloat();
   }
   else{
      orpValue = 9999;
   }
}

void loop(){                   
  Serial.println("Starting new loop...");

  if (logfile) {
    logfile.println("Starting new loop...");
    logfile.flush();
  }

  DateTime now = RTC.now();
  loopConductivity();
  loopOrp();
  
  /////////////////////////////////////////////////////////////////////
  //  Send all data to serial output 
  /////////////////////////////////////////////////////////////////////
  if (logfile) {
    logfile.print(now.unixtime());
    logfile.print(",");
    logfile.print(tdsValue);
    logfile.print(",");
    logfile.print(salinityValue);
    logfile.print(",");
    logfile.print(orpValue);
    logfile.print(",");       
    logfile.print("\n");
  }
  
  Serial.print(now.unixtime());
  Serial.print(",");
  Serial.print(tdsValue);
  Serial.print(",");
  Serial.print(salinityValue);
  Serial.print(",");
  Serial.print(orpValue);
  Serial.print(",");       
  Serial.print("\n");
  
  if (logfile) {
    logfile.flush();
  }

  memset((void *)&payload, 0, sizeof(payload));
  payload.kind = 1;
  payload.v1 = tdsValue;
  payload.v2 = salinityValue;
  payload.v3 = orpValue;
  payload.time = now.unixtime();
  longDelayAndAttemptToSendPacket(60L * 1000L * 60L * 6L);

  Serial.println("Done");
}
