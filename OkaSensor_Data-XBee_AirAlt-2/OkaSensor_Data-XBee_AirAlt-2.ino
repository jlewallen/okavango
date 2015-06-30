#include <XBee.h>
#include <SD.h>
#include <SPI.h>
#include <RTClib.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MPL115A2.h>
#include <SoftwareSerial.h>        // include the software serial library to add an aditional serial ports to talk to the Atlas units

#define SD_BOARD_PIN     10

SoftwareSerial xbeeSerial(2, 3); 
// Initialize DHT sensor for normal 16mhz Arduino
Adafruit_MPL115A2 mpl115a2;
RTC_DS1307 RTC; // define the Real Time Clock object
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

// SH + SL Address of receiving XBee
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

void setup(){             
    xbeeSerial.begin(9600);        
    Serial.begin(115200);      
    xbee.setSerial(xbeeSerial);

    Wire.begin();    
    RTC.begin();
    mpl115a2.begin();

    openLogFile();
}

void loop(){                   
  Serial.println("Starting new loop...");

  logfile.println("Starting new loop...");
  logfile.flush();

  DateTime now = RTC.now();
  
  float pressure = mpl115a2.getPressure(); 

  /////////////////////////////////////////////////////////////////////
  //  Send all data to serial output 
  /////////////////////////////////////////////////////////////////////
  logfile.print(now.unixtime());
  logfile.print(",");
  logfile.print(pressure);
  logfile.print("\n");

  Serial.print(now.unixtime());
  Serial.print(",");
  Serial.print(pressure);
  Serial.print("\n");
  
  logfile.flush();

  memset((void *)&payload, 0, sizeof(payload));
  payload.kind = 4;
  payload.time = now.unixtime();
  payload.v1 = pressure;
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
