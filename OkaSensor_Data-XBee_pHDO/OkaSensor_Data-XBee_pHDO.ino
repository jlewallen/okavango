#include <XBee.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_Sensor.h>
#include <SoftwareSerial.h>

#define XBEE_DESTINATION_HIGH 0x0013A200
#define XBEE_DESTINATION_LOW 0x409F2937
#include "XBeeUtilities.h"

#define PH_RXPIN         4 // set the pH sensor RX pin (labeled "TX" on pH board)
#define PH_TXPIN         5 // set the pH sensor TX pin (labeled "RX" on pH board)
#define D_O_RXPIN        6 // set the dissolved oxygen RX pin (labeled "TX" on DO board)
#define D_O_TXPIN        7 // set the dissolved oxygen TX pin (labeled "RX" on DO board)
#define SD_BOARD_PIN     10

float phValue;
float doValue;

SoftwareSerial pH(PH_RXPIN, PH_TXPIN);
SoftwareSerial d_o(D_O_RXPIN, D_O_TXPIN); 
SoftwareSerial xbeeSerial(2, 3); 
RTC_DS1307 RTC;
File logfile;

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
    xbeeSerial.begin(9600);
    Serial.begin(115200);      
    xbee.setSerial(xbeeSerial);
    
    Wire.begin();
    RTC.begin();
    
    openLogFile();

    configureSleepMode();
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

void loopDissolvedOxygen()
{
   String rawSensor = "";               
   boolean gotEnd = false; 

   d_o.begin(9600);
   delay(1000);
   d_o.print("r \r");          // send string "r" with a carriage return "\r" to take one reading
   delay(3000);                // it takes a while for the pH sensor to respond

   while (d_o.available() > 0) {     
      char c = (char)d_o.read();                 
      rawSensor += c;                              
      if (c == '\r') {
        gotEnd = true;
       }
      delay(50);
   }         
   d_o.end();
   
   if (gotEnd){
      int n = rawSensor.indexOf('*');
      String doString = rawSensor.substring(0, n);
      doValue = doString.toFloat();
   }
   else{
      doValue = 9999;
   } 
}

void loop(){                   
  Serial.println("Starting new loop...");

  logfile.println("Starting new loop...");
  logfile.flush();

  DateTime now = RTC.now();
  loopPh();
  loopDissolvedOxygen();

  /////////////////////////////////////////////////////////////////////
  //  Send all data to serial output 
  /////////////////////////////////////////////////////////////////////
  logfile.print(now.unixtime());
  logfile.print(",");
  logfile.print(phValue);
  logfile.print(",");
  logfile.print(doValue);
  logfile.print(",");
  logfile.print("\n");

  Serial.print(now.unixtime());
  Serial.print(",");
  Serial.print(phValue);
  Serial.print(",");
  Serial.print(doValue);
  Serial.print(",");
  Serial.print("\n");
  
  logfile.flush();

  memset((void *)&payload, 0, sizeof(payload));
  payload.kind = 0;
  payload.v1 = phValue;
  payload.v2 = doValue;
  payload.time = now.unixtime();
  longDelayAndAttemptToSendPacket(60L * 1000L * 60L * 6L);
  
  Serial.println("Done");
}
