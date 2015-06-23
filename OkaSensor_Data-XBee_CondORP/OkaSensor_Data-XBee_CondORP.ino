#include <XBee.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_Sensor.h>
#include <SoftwareSerial.h>        // include the software serial library to add an aditional serial ports to talk to the Atlas units
#define LOG_INTERVAL  5000 // mills between entries (reduce to take more/faster data)
#define SYNC_INTERVAL 5000 // mills between calls to flush() - to write data to the card
uint32_t syncTime = 0; // time of last sync()
#define ECHO_TO_SERIAL   0 // echo data to serial port
#define WAIT_TO_START    0 // Wait for serial input in setup()
#define cond_rxpin 4               // set the conductivity (EC sensor) RX pin (labeled "TX" on EC board)
#define cond_txpin 5               // set the conductivity (EC sensor) TX pin (labeled "RX" on EC board)
#define orp_rxpin 6              // set the ORP RX pin (labeled "TX" on ORP board) - NEED TO CHANGE THIS FOR THE SD SHIELD
#define orp_txpin 7              // set the ORP TX pin (labeled "RX" on ORP board) - NEED TO CHANGE THIS FOR THE SD SHIELD

// Create an instance of the softwareSerial class for each sensor
SoftwareSerial cond(cond_rxpin, cond_txpin);  
SoftwareSerial orp(orp_rxpin, orp_txpin);
SoftwareSerial xbeeSerial(2, 3); 

RTC_DS1307 RTC; // define the Real Time Clock object
const int chipSelect = 10;

File logfile;

XBee xbee = XBee();

// Set variables
String sensorstring_cond = "";               
boolean sensor_stringcomplete_cond = false; 
String sensorstring_orp = "";               
boolean sensor_stringcomplete_orp = false; 
String gotcha;
int i;
int j;
int k;
int m;
int n;
String cond_final;
String orp_final;

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

void loopOrp() 
{
   orp.begin(9600);           // baud rate for sensor = 9600 (known)
   delay(1000);
   orp.print("r \r");          // send string "r" with a carriage return "\r" to take one reading
   delay(3000);               // it takes a while for the sensor to respond

   while (orp.available() > 0) {     
      char inchar_orp = (char)orp.read();                 
      sensorstring_orp += inchar_orp;                              
      if (inchar_orp == '\r') {sensor_stringcomplete_orp = true;}
      delay(50);
   }         
   orp.end();
   
   if (sensor_stringcomplete_orp){
      //Serial.print("orp: ");
      //Serial.println(sensorstring_orp);
      int n = sensorstring_orp.indexOf('*'); // Find end 
 //     Serial.println(n);
      for(k=0; k<=n-1; k++) {                    // for pH value
//         m=k-i;
        gotcha += sensorstring_orp[k];           // Build up "gotcha" variable with TDS reading
        orp_final = gotcha;
      }
//      orp_final = sensorstring_orp;
      sensorstring_orp = "";
      gotcha = "";
      sensor_stringcomplete_orp = false;
   }
   else{
     //Serial.println("orp: 9999");
     //Serial.print("9999,");
     orp_final = "9999";
     sensorstring_orp = "";
     sensor_stringcomplete_orp = false;
   }
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
  loopOrp();
  
  // delay(1000); This may not be necessary 

  /////////////////////////////////////////////////////////////////////
  //  Send all data to serial output 
  /////////////////////////////////////////////////////////////////////
  logfile.print(now.unixtime());
  logfile.print(",");
  logfile.print(cond_final);
  logfile.print(orp_final);
  logfile.print(",");       
  logfile.print("\n");

  Serial.print(now.unixtime());
  Serial.print(",");
  Serial.print(cond_final);
  Serial.print(",");
  Serial.print(orp_final);
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
