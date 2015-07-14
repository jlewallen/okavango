#include <XBee.h>
#include <SoftwareSerial.h>

#define XBEE_DESTINATION_HIGH 0x0013A200
#define XBEE_DESTINATION_LOW 0x40E677BF
#include "XBeeUtilities.h"

uint32_t counter = 0;

void setup() {
  Serial.begin(115200);
  xbee.setSerial(xbeeSerial);

  Serial.println("Starting...");
  configureSleepMode();
}

void loop() {
  Serial.print("Sending ");
  Serial.println(counter);

  memset((void *)&payload, 0, sizeof(payload));
  longDelayAndAttemptToSendPacket(60L * 1000L * 30L);
  resetArduino();
  
  counter++;
}
