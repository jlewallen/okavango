#include <XBee.h>
#include <SoftwareSerial.h>

#define XBEE_DESTINATION_HIGH 0x0013A200
#define XBEE_DESTINATION_LOW 0x40E677BF
#include "XBeeUtilities.h"

SoftwareSerial xbeeSerial(2, 3); 

uint32_t counter = 0;

void setup() {
  xbeeSerial.begin(9600);
  Serial.begin(115200);
  xbee.setSerial(xbeeSerial);

  Serial.println("Starting...");
  configureSleepMode();
}

void loop() {
  Serial.print("Sending ");
  Serial.println(counter);

  memset((void *)&payload, 0, sizeof(payload));
  longDelayAndAttemptToSendPacket(60L * 1000L * 60L);
  
  counter++;
}
