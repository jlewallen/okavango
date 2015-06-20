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
#define ECHO_TO_SERIAL   1 // echo data to serial port
#define WAIT_TO_START    0 // Wait for serial input in setup()
#define ONE_WIRE_BUS 2             // Water temp wire is plugged into pin 2 on the Arduino
#define cond_rxpin 3               // set the conductivity (EC sensor) RX pin (labeled "TX" on EC board)
#define cond_txpin 4               // set the conductivity (EC sensor) TX pin (labeled "RX" on EC board)
#define pH_rxpin 5                 // set the pH sensor RX pin (labeled "TX" on pH board)
#define pH_txpin 6                 // set the pH sensor TX pin (labeled "RX" on pH board)
#define d_o_rxpin 7              // set the dissolved oxygen RX pin (labeled "TX" on DO board)
#define d_o_txpin 8              // set the dissolved oxygen TX pin (labeled "RX" on DO board)
//#define orp_rxpin 9              // set the ORP RX pin (labeled "TX" on ORP board) - NEED TO CHANGE THIS FOR THE SD SHIELD
//#define orp_txpin 10              // set the ORP TX pin (labeled "RX" on ORP board) - NEED TO CHANGE THIS FOR THE SD SHIELD
#define DHTPIN 11                 // what pin we're connected to
#define DHTTYPE DHT22             // set type of sensor to DHT 22  (AM2302)

// Create an instance of the softwareSerial class for each sensor
SoftwareSerial cond(cond_rxpin, cond_txpin);  
SoftwareSerial pH(pH_rxpin, pH_txpin);
SoftwareSerial d_o(d_o_rxpin, d_o_txpin); 
//SoftwareSerial orp(orp_rxpin, orp_txpin); - NEED TO CHANGE THIS FOR THE SD SHIELD

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
String sensorstring_cond = "";               
boolean sensor_stringcomplete_cond = false; 
String sensorstring_pH = "";               
boolean sensor_stringcomplete_pH = false; 
String sensorstring_d_o = "";               
boolean sensor_stringcomplete_d_o = false; 
String sensorstring_orp = "";               
boolean sensor_stringcomplete_orp = false; 
String gotcha;
int i;
int j;
int k;
int m;
int n;
String cond_final;
String pH_final;
String d_o_final;
String orp_final;

// we are going to send two floats of 4 bytes each
uint8_t payload[4 * 4 + 1] = {};

union u_tag {
    uint8_t b[4];
    float fval;
} u;

// SH + SL Address of receiving XBee
XBeeAddress64 addr64 = XBeeAddress64(0x0013a200, 0x406F4973); //this needs to be updated for the correct XBee
ZBTxRequest zbTx = ZBTxRequest(addr64, payload, sizeof(payload));
ZBTxStatusResponse txStatus = ZBTxStatusResponse();

void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);
  while(1);
}

void setup(){             
      
    Serial.begin(115200);      //Can we change this to 9600 for XBee sake?
    xbee.setSerial(Serial);
    RTC.begin();
    sensors.begin();             // Start up the library for water sensor
    dht.begin();                 // Start up the library for air temp/humidity sensor
    bmp.begin();      // Start up the library for the altitude sensor

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

void loopConductivity()
{
   // conductivity sensor outputs 3 numbers separated by commas
   cond.begin(9600);         // baud rate for cond sensor = 9600 (known)
   delay(1000);               // allow for sensor to start communicating
   cond.print("r \r");        // send string "r" with a carriage return "\r" to take one reading
   delay(1000);               // wait for reading to be read and sent
   
   while (cond.available() > 0) {                                   // message is being sent                      
      char inchar_cond = (char)cond.read();                         // string msg is read as individual characters           
      sensorstring_cond += inchar_cond;      // adding each individual character to whole "sentence"  
      if (inchar_cond == '\r') {sensor_stringcomplete_cond = true;} // if carriage return is sensed then complete string
      delay(50);
   }            
   cond.end();   

   if (sensor_stringcomplete_cond) {
      int i = sensorstring_cond.indexOf(',');      // Find location of first ","
      int j = sensorstring_cond.indexOf(',', i+1); // Find location of second ","
      int n = sensorstring_cond.indexOf('*', j+1); // Find end 
      for(k=i+1; k<=n-1; k++) {                    // for middle number (TDS) and second to last (salinity)
//         m=k-i;
         gotcha += sensorstring_cond[k];           // Build up "gotcha" variable with TDS reading
         cond_final = gotcha;
      }
      //Serial.print("Conductivity (PPM): ");      // Use if printing to screen, not databoat
      //Serial.println(gotcha);                    // Print out just the TDS reading (PPM)
      //Serial.print(gotcha);
      //Serial.print(",");
      sensorstring_cond = "";                      // Reset the sensorstring, gotcha and _cond variables for next run
      gotcha = "";
      sensor_stringcomplete_cond = false;
   }   
   else{                                           // if no variables are found within the pickup call = set variable to 9999
     //Serial.println("Conductivity (PPM): 9999");
     //Serial.print("9999,");
     cond_final = "9999,9999";                     // Input two 9999s for TDS, Salinity if sensor is giving incorrect values
     sensorstring_cond = "";                       // Reset the sensorstring variable for next run
     sensor_stringcomplete_cond = false;
   }   
}

void loopPh()
{
   // same code to run conductivity above except pH only outputs 1 number vs 3 for conductivity sensor
   pH.begin(9600);           // baud rate for pH sensor = 9600 (known)
   delay(1000);
   pH.print("r \r");          // send string "r" with a carriage return "\r" to take one reading
   delay(3000);               // it takes a while for the pH sensor to respond

   while (pH.available() > 0) {                     // message is being sent              
      char inchar_pH = (char)pH.read();             // string msg is read as individual characters       
      sensorstring_pH += inchar_pH;                              
      if (inchar_pH == '\r') {sensor_stringcomplete_pH = true;}
      delay(50);
   }         
   pH.end();
   
   if (sensor_stringcomplete_pH){
      //Serial.print("pH: ");
      //Serial.println(sensorstring_pH);
      int n = sensorstring_pH.indexOf('*'); // Find end 
      for(k=0; k<=n-1; k++) {                    // for pH value
//         m=k-i;
         gotcha += sensorstring_pH[k];           // Build up "gotcha" variable with TDS reading
         pH_final = gotcha;
      }
//      pH_final = sensorstring_pH;
      sensorstring_pH = "";
      gotcha = "";
      sensor_stringcomplete_pH = false;
   }
   else{
     //Serial.println("pH: 9999");
     //Serial.print("9999,");
     pH_final = "9999";
     sensorstring_pH = "";
     sensor_stringcomplete_pH = false;
   }
}

void loopDissolvedOxygen()
{
     d_o.begin(9600);           // baud rate for sensor = 9600 (known)
   delay(1000);
   d_o.print("r \r");          // send string "r" with a carriage return "\r" to take one reading
   delay(3000);               // it takes a while for the pH sensor to respond

   while (d_o.available() > 0) {     
      char inchar_d_o = (char)d_o.read();                 
      sensorstring_d_o += inchar_d_o;                              
      if (inchar_d_o == '\r') {sensor_stringcomplete_d_o = true;}
      delay(50);
   }         
   d_o.end();
   
   if (sensor_stringcomplete_d_o){
      //Serial.print("d_o: ");
      //Serial.println(sensorstring_d_o);
      int n = sensorstring_d_o.indexOf('*'); // Find end 
 //     Serial.println(n);
      for(k=0; k<=n-1; k++) {                    // for pH value
//         m=k-i;
        gotcha += sensorstring_d_o[k];           // Build up "gotcha" variable with TDS reading
        d_o_final = gotcha;
      }
//      d_o_final = sensorstring_d_o;
      sensorstring_d_o = "";
      gotcha = "";
      sensor_stringcomplete_d_o = false;
   }
   else{
     //Serial.println("d_o: 9999");
     //Serial.print("9999,");
     d_o_final = "9999";
     sensorstring_d_o = "";
     sensor_stringcomplete_d_o = false;
   } 
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

//void loopOrp() // NEED TO CHANGE THIS FOR THE SD SHIELD
//{
//   orp.begin(9600);           // baud rate for sensor = 9600 (known)
//   delay(1000);
//   orp.print("r \r");          // send string "r" with a carriage return "\r" to take one reading
//   delay(3000);               // it takes a while for the sensor to respond
//
//   while (orp.available() > 0) {     
//      char inchar_orp = (char)orp.read();                 
//      sensorstring_orp += inchar_orp;                              
//      if (inchar_orp == '\r') {sensor_stringcomplete_orp = true;}
//      delay(50);
//   }         
//   orp.end();
//   
//   if (sensor_stringcomplete_orp){
//      //Serial.print("orp: ");
//      //Serial.println(sensorstring_orp);
//      int n = sensorstring_orp.indexOf('*'); // Find end 
// //     Serial.println(n);
//      for(k=0; k<=n-1; k++) {                    // for pH value
////         m=k-i;
//        gotcha += sensorstring_orp[k];           // Build up "gotcha" variable with TDS reading
//        orp_final = gotcha;
//      }
////      orp_final = sensorstring_orp;
//      sensorstring_orp = "";
//      gotcha = "";
//      sensor_stringcomplete_orp = false;
//   }
//   else{
//     //Serial.println("orp: 9999");
//     //Serial.print("9999,");
//     orp_final = "9999";
//     sensorstring_orp = "";
//     sensor_stringcomplete_orp = false;
//   }
//}

sensors_event_t event;
  
float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;

void loopAltitude()
{
  bmp.getEvent(&event);
//  float temperature;
//  bmp.getTemperature(&temperature);
  
///* Display the results (barometric pressure is measure in hPa) */
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
  loopConductivity();
  loopPh();
  loopDissolvedOxygen();
//  loopOrp();
  loopWaterTemperature();
//  loopAirTemperatureAndHumidity();
  loopAltitude();
  
  // delay(1000); This may not be necessary 

  /////////////////////////////////////////////////////////////////////
  //  Send all data to serial output 
  /////////////////////////////////////////////////////////////////////
  logfile.print(now.unixtime());
  logfile.print(",");
  logfile.print(cond_final);
  logfile.print(",");
  logfile.print(pH_final);
  logfile.print(",");
  logfile.print(d_o_final);
  logfile.print(",");
//  logfile.print(orp_final);
//  logfile.print(",");       
  logfile.print(sensors.getTempCByIndex(0)); 
  logfile.print(",");
  logfile.print(h);
  logfile.print(",");
  logfile.print(t);
  logfile.print(",");
  logfile.print(bmp.pressureToAltitude(seaLevelPressure, event.pressure));
  logfile.print(",");
  logfile.print(event.pressure);
  logfile.print(",");
  logfile.print("\n");

  Serial.print(now.unixtime());
  Serial.print(",");
  Serial.print(cond_final);
  Serial.print(",");
  Serial.print(pH_final);
  Serial.print(",");
  Serial.print(d_o_final);
  Serial.print(",");
//  Serial.print(orp_final);
//  Serial.print(",");       
  Serial.print(sensors.getTempCByIndex(0)); 
  Serial.print(",");
  Serial.print(h);
  Serial.print(",");
  Serial.print(t);
  Serial.print(",");
  Serial.print(bmp.pressureToAltitude(seaLevelPressure, event.pressure));
  Serial.print(",");
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
  
  if (xbee.readPacket(500)) {
      if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
        xbee.getResponse().getZBTxStatusResponse(txStatus);
        if (txStatus.getDeliveryStatus() == SUCCESS) {
        } else {
        }
      }
    } else if (xbee.getResponse().isError()) {
    } else {
    }
  //delay(10000); // delay for 10 seconds but ultimately 
}
