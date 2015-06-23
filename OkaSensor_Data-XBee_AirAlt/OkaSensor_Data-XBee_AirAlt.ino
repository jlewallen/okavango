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
#include <Adafruit_BMP085_U.h>
#include <SoftwareSerial.h>        // include the software serial library to add an aditional serial ports to talk to the Atlas units
#define LOG_INTERVAL  5000 // mills between entries (reduce to take more/faster data)
#define SYNC_INTERVAL 5000 // mills between calls to flush() - to write data to the card
uint32_t syncTime = 0; // time of last sync()
#define ECHO_TO_SERIAL   0 // echo data to serial port
#define WAIT_TO_START    0 // Wait for serial input in setup()
#define ONE_WIRE_BUS 6             // Water temp wire is plugged into pin 2 on the Arduino
#define DHTPIN 7                 // what pin we're connected to
#define DHTTYPE DHT22             // set type of sensor to DHT 22  (AM2302)

// Create an instance of the softwareSerial class for each sensor
SoftwareSerial xbeeSerial(2, 3); 

// Setup a oneWire instance to communicate with any OneWire devices 
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// Initialize DHT sensor for normal 16mhz Arduino
DHT dht(DHTPIN, DHTTYPE);

Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

RTC_DS1307 RTC; // define the Real Time Clock object
const int chipSelect = 10;

File logfile;

XBee xbee = XBee();

// Set variables
String gotcha;
int i;
int j;
int k;
int m;
int n;

// we are going to send two floats of 4 bytes each
uint8_t payload[4 * 4 + 1] = {};

union u_tag {
    uint8_t b[4];
    float fval;
} u;

// SH + SL Address of receiving XBee
XBeeAddress64 addr64 = XBeeAddress64(0x0013A200, 0x40C6746A); //this needs to be updated for the correct XBee Test configuration: 13A200  40C6746A
ZBTxRequest zbTx = ZBTxRequest(addr64, payload, sizeof(payload));
ZBTxStatusResponse txStatus = ZBTxStatusResponse();

void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);
  while(1);
}

void setup(){             
    xbeeSerial.begin(9600);        
    Serial.begin(115200);      
    xbee.setSerial(xbeeSerial);

    Wire.begin();    
    RTC.begin();
    
    sensors.begin();             // Start up the library for water sensor
    dht.begin();                 // Start up the library for air temp/humidity sensor
    bmp.begin();      // Start up the library for the altitude sensor

    Serial.println(RTC.now().unixtime());

    pinMode(10, OUTPUT);
  
    // see if the card is present and can be initialized:
    if (!SD.begin(chipSelect)) {
      error("Card failed, or not present");
    }
  
    // create a new file
    char filename[] = "LOGGER00.CSV";
    for (uint8_t i = 0; i < 100; i++) {
      filename[6] = i/10 + '0';
      filename[7] = i%10 + '0';
      if (! SD.exists(filename)) {
        // only open a new file if it doesn't exist
        logfile = SD.open(filename, FILE_WRITE); 
        break;  // leave the loop!
    }
  }
  
  if (! logfile) {
    error("couldnt create file");
  }
  
  Serial.print("Logging to: ");
  Serial.println(filename);
  logfile.println("Ready");
  logfile.flush();
}

float h, t, f;

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
  
/* Display the results (barometric pressure is measure in hPa) */
//  if (event.pressure)
//  {
//    /* Display atmospheric pressue in hPa */
//    Serial.print("Pressure:    ");
//    Serial.print(event.pressure);
//    Serial.println(" hPa");
//     
//    /* First we get the current temperature from the BMP085 */
//    float temperature;
//    bmp.getTemperature(&temperature);
//    Serial.print("Temperature: ");
//    Serial.print(temperature);
//    Serial.println(" C");
//
//    /* Then convert the atmospheric pressure, and SLP to altitude         */
//    /* Update this next line with the current SLP for better results      */
//    float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;
//    Serial.print("Altitude:    "); 
//    Serial.print(bmp.pressureToAltitude(seaLevelPressure,
//                                        event.pressure)); 
//    Serial.println(" m");
//    Serial.println("");
//  }
//  else
//  {
//    Serial.println("9999");
//  }
}

void addToPayload(int position, float value) {
  u.fval = value;
  for (int i=0;i<4;i++){
    payload[position * 4 + i] = u.b[i];
  }
}

void loop(){                   
  Serial.println("Starting new loop...");

  logfile.println("Starting new loop...");
  logfile.flush();

  DateTime now = RTC.now();
//  loopWaterTemperature();
  loopAirTemperatureAndHumidity();
  loopAltitude();
  
  // delay(1000); This may not be necessary 

  /////////////////////////////////////////////////////////////////////
  //  Send all data to serial output 
  /////////////////////////////////////////////////////////////////////
  logfile.print(now.unixtime());
  logfile.print(",");
//  logfile.print(sensors.getTempCByIndex(0)); 
//  logfile.print(",");
  logfile.print(h);
  logfile.print(",");
  logfile.print(t);
  logfile.print(",");
//  logfile.print(bmp.pressureToAltitude(seaLevelPressure, event.pressure));
//  logfile.print(",");
  logfile.print(event.pressure);
  logfile.print(",");
  logfile.print("\n");

  Serial.print(now.unixtime());
  Serial.print(",");
//  Serial.print(sensors.getTempCByIndex(0)); 
//  Serial.print(",");
  Serial.print(h);
  Serial.print(",");
  Serial.print(t);
  Serial.print(",");
//  Serial.print(bmp.pressureToAltitude(seaLevelPressure, event.pressure));
//  Serial.print(",");
  Serial.print(event.pressure);
  Serial.print(",");
  Serial.print("\n");
  
  logfile.flush();

  memset(payload, 0, sizeof(payload));
  payload[sizeof(payload) - 1] = 0; // This determines the contents of the packet. Receiver looks at this to tell which floats are in the packet.
  addToPayload(0, 0.0f);
  addToPayload(1, 1.0f);
  addToPayload(2, 2.0f);
  addToPayload(3, 3.0f);
  xbee.send(zbTx);
  
  if (xbee.readPacket(5000)) {
      if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
        xbee.getResponse().getZBTxStatusResponse(txStatus);
        if (txStatus.getDeliveryStatus() == SUCCESS) {
          Serial.println("Message deliveried.");
        } else {
          Serial.println("Message NOT deliveried.");          
        }
      }
    } else if (xbee.getResponse().isError()) {
      Serial.println("Got error response.");          
    } else {
      Serial.println("No response packet.");          
    }
  //delay(10000); // delay for 10 seconds but ultimately 
}
