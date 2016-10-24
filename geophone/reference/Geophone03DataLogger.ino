#include <SD.h>
#include <Wire.h>
#include "RTClib.h"

// define the led pins for visual output
#define redLEDpin 3
#define greenLEDpin 2

// visual output key for the debug LEDs [in the corner, marked LED1 and LED2]:
// at start, when just loading the sketch, to verify that the sd card loaded correctly, it will:
// alternate between red and green for about 2 seconds
// to verify that it's logging data, the red SD led [marked SD] blinks every write cycle [each second]
//   every fifth write cycle the green led will blink
// if the sd card does not load properly, try pulling it out and putting it back in, then wait a second, then reset the arduino
// or try unplugging the arduino and then plugging back in and resetting


RTC_DS1307 RTC; // define the Real Time Clock object
DateTime now;

// the logging file
File logfile;

// note.. new file making is hard coded below...

// ALL DATA IS FLUSHED EVERY SECOND.  SINCE THERE seem to be at most 817 readings per second, make an array to store those readings
int dataArray[1300]; // increased in size in the setup as dataPerSecond * logInterval
// this array holds the data before being flushed to the logger

// some holder vars
long dataCount = 0;
int sec = 0;
int lastSec = -1;
int minute = 0;
int lastMinute = 0;
long unix = 0;

// geophone var
const int geophoneInput = A5; // pin where geophone is plugged into

int dataFlushCount = 0; // used for blinking the green led every n counts

String fileName = "";
int fileNameLength = 10; // gets replaced with the length of the file name each time a new one is made
int i = 0; // just an i


void error(char *str) {
  Serial.print("error: ");
  Serial.println(str);
  // red LED indicates error
  //digitalWrite(redLEDpin, HIGH);
  while (1);
} // end error


// get a new file name
void makeNewFileName() {
  now = RTC.now();
  // nope... too long
  //fileName = String(now.year(), DEC) + "" +  (now.month() < 10 ? "0" : "") + now.month() + "" +  (now.day() < 10 ? "0" : "") + now.day() + "" +  (now.hour() < 10 ? "0" : "") + now.hour() + "" +  (now.minute() < 10 ? "0" : "") + now.minute() + "" +  (now.second() < 10 ? "0" : "") + now.second() + ".csv";
  //1451624400 is epoch local time of 1/1/2016 0:0:0 // to reduce the name length.  it can only seem to handle pizza123.csv as longest length
  fileName = String(now.unixtime() - 1451624400, DEC) + ".csv";
  fileNameLength = fileName.length() + 1;
} // end makeNewFileName

//
// print a timestamp in the log file
void printTime() {
  logfile.print("time-");
  logfile.print(now.unixtime());
  logfile.print(" ");
  logfile.print(now.year(), DEC);
  logfile.print("/");
  logfile.print(now.month(), DEC);
  logfile.print("/");
  logfile.print(now.day(), DEC);
  logfile.print(" ");
  logfile.print(now.hour(), DEC);
  logfile.print(":");
  logfile.print(now.minute(), DEC);
  logfile.print(":");
  logfile.println(now.second(), DEC);
} // end printTime

//
// this is when the data is written to the log
void flushDataArray() {
  if (dataFlushCount % 5 == 0) {
    digitalWrite(greenLEDpin, HIGH);
  }
  for (i = 0; i < dataCount; i++) {
    logfile.println(dataArray[i]);
  }
  if (dataFlushCount % 5 == 0) {
    digitalWrite(greenLEDpin, LOW);
  }
  dataCount = 0; // reset the counter for the array spot
  dataFlushCount++; // increase the flush count
} // end flushDataArray

//
void makeNewFile() {
  makeNewFileName();
  Serial.println("making new file!");
  Serial.print(" and name is: " );
  Serial.println(fileName);
  char a[fileNameLength];
  fileName.toCharArray(a, fileNameLength);
  logfile = SD.open(a, FILE_WRITE);
  if (! logfile) {
    error("couldnt create file");
  }
  // make the header
  dataFlushCount = 0; // reset the flush count.. this is used just for blinking
} // end makeNewFile

//
void flushLog() {
  Serial.println("flushing log");
  // blink LED to show we are syncing data to the card & updating FAT!
  digitalWrite(redLEDpin, HIGH);
  digitalWrite(greenLEDpin, HIGH);
  logfile.flush();
  digitalWrite(redLEDpin, LOW);
  digitalWrite(greenLEDpin, LOW);
} // end flushLog


void setup() {
  Serial.begin(9600);
  
  // use debugging LEDs
  pinMode(redLEDpin, OUTPUT);
  pinMode(greenLEDpin, OUTPUT);

  // initialize the SD card
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);

  // see if the card is present and can be initialized:
  if (!SD.begin(10, 11, 12, 13)) {
    error("Card failed, or not present");
  }
  // IF EVERYTHING IS GOOD, RED AND GREEN LEDS WILL ALTERNATE FOR A BIT
  Serial.println("card initialized.");
  digitalWrite(redLEDpin, HIGH);
  digitalWrite(greenLEDpin, LOW);
  delay(650);
  digitalWrite(redLEDpin, LOW);
  digitalWrite(greenLEDpin, HIGH);
  delay(650);
  digitalWrite(redLEDpin, HIGH);
  digitalWrite(greenLEDpin, LOW);
  delay(650);
  digitalWrite(redLEDpin, LOW);
  digitalWrite(greenLEDpin, HIGH);
  delay(650);
  digitalWrite(redLEDpin, HIGH);
  digitalWrite(greenLEDpin, LOW);
  delay(650);
  digitalWrite(redLEDpin, LOW);
  digitalWrite(greenLEDpin, HIGH);
  delay(650);
  digitalWrite(redLEDpin, HIGH);
  digitalWrite(greenLEDpin, LOW);
  delay(650);
  digitalWrite(redLEDpin, LOW);
  digitalWrite(greenLEDpin, HIGH);
  delay(650);
  digitalWrite(redLEDpin, HIGH);
  digitalWrite(greenLEDpin, LOW);
  delay(650);
  digitalWrite(redLEDpin, LOW);

  // connect to RTC
  Wire.begin();
  if (!RTC.begin()) {
    // logfile.println("RTC failed");
  }

  // make a new file
  makeNewFile();

  // establish same minute so it doesnt auto-write file the first time
  minute = now.minute();
  lastMinute = minute;
} // end setup

void loop() {
  // fetch the time
  now = RTC.now();

  // log time
  unix = now.unixtime(); // seconds since 1/1/1970
  sec = now.second();
  minute = now.minute();
  
  if (sec != lastSec) {
    lastSec = sec;
    printTime(); // every second
    // FLUSH DATA EVERY SECOND
    flushDataArray();
    
    // every m minutes flush the file and make a new one
    if (minute % 3 == 0 && lastMinute != minute) {
      flushLog();
      makeNewFile();
      lastMinute = minute;
    }
  }

  readGeo(geophoneInput);
} // end loop

//
// read in the data from the geophone
void readGeo(int geoPin) {
  if (dataCount < 1299) {
    dataArray[dataCount] = analogRead(geophoneInput);
  }
  dataCount++;
} // end readAxis


