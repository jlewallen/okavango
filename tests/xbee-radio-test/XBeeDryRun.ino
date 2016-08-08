#include <XBee.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

#define XBEE_DESTINATION_HIGH 0x0013A200
#define XBEE_DESTINATION_LOW 0x40E677BF
#include "XBeeUtilities.h"

void setup() {
  Serial.begin(115200);
  xbee.setSerial(xbeeSerial);

  Serial.println("Starting...");
  configureSleepMode();
}

void loop() {
  Serial.print("Sending ");

  memset((void *)&payload, 0, sizeof(payload));
  longDelayAndAttemptToSendPacket(60L * 1000L);
  resetArduino();
}
