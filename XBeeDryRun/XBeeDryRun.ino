#include <XBee.h>
#include <SoftwareSerial.h>
#include "XBeeUtilities.h"

SoftwareSerial xbeeSerial(2, 3); 

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
  longDelayAndAttemptToSendPacket(60L * 1000L * 5L);
  
  counter++;
}
