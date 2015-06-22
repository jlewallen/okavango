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
#define pH_rxpin 4                 // set the pH sensor RX pin (labeled "TX" on pH board)
#define pH_txpin 5                 // set the pH sensor TX pin (labeled "RX" on pH board)
#define d_o_rxpin 6              // set the dissolved oxygen RX pin (labeled "TX" on DO board)
#define d_o_txpin 7              // set the dissolved oxygen TX pin (labeled "RX" on DO board)

// Create an instance of the softwareSerial class for each sensor 
SoftwareSerial pH(pH_rxpin, pH_txpin);
SoftwareSerial d_o(d_o_rxpin, d_o_txpin); 
SoftwareSerial xbeeSerial(2, 3); 

RTC_DS1307 RTC; // define the Real Time Clock object
const int chipSelect = 10;

File logfile;

XBee xbee = XBee();

// Set variables
String sensorstring_pH = "";               
boolean sensor_stringcomplete_pH = false; 
String sensorstring_d_o = "";               
boolean sensor_stringcomplete_d_o = false; 
String gotcha;
int i;
int j;
int k;
int m;
int n;
String pH_final;
String d_o_final;

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
    Serial.begin(115200);      //Can we change this to 9600 for XBee sake?
    xbee.setSerial(xbeeSerial);
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
  loopPh();
  loopDissolvedOxygen();
  
  // delay(1000); This may not be necessary 

  /////////////////////////////////////////////////////////////////////
  //  Send all data to serial output 
  /////////////////////////////////////////////////////////////////////
  logfile.print(now.unixtime());
  logfile.print(",");
  logfile.print(pH_final);
  logfile.print(",");
  logfile.print(d_o_final);
  logfile.print(",");
  logfile.print("\n");

  Serial.print(now.unixtime());
  Serial.print(",");
  Serial.print(pH_final);
  Serial.print(",");
  Serial.print(d_o_final);
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
          Serial.println("Message deliveried.");
        } else {
          Serial.println("Message NOT deliveried.");          
        }
      }
    } else if (xbee.getResponse().isError()) {
      Serial.println("Got error response.");          
    } else {
      Serial.println("Got unknown response.");          
    }
  //delay(10000); // delay for 10 seconds but ultimately 
}
