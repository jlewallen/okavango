/*
 * This example check if the firmware loaded on the ATWINC1500 is updated.
 *
 * Circuit:
 * - Feather M0 WiFi (WINC1500), WiFi 101 shield, or WINC1500 Breakout
 *
 * Created 29 July 2015 by Cristian Maglie
 * This code is in the public domain.
 */
#include <SPI.h>
#include <Adafruit_WINC1500.h>

// Define the WINC1500 board connections below.
// If you're following the Adafruit WINC1500 board
// guide you don't need to modify these:
#define WINC_CS   8
#define WINC_IRQ  7
#define WINC_RST  4
#define WINC_EN   2     // or, tie EN to VCC
// The SPI pins of the WINC1500 (SCK, MOSI, MISO) should be
// connected to the hardware SPI port of the Arduino.
// On an Uno or compatible these are SCK = #13, MISO = #12, MOSI = #11.
// On an Arduino Zero use the 6-pin ICSP header, see:
//   https://www.arduino.cc/en/Reference/SPI

// Setup the WINC1500 connection with the pins above and the default hardware SPI.
Adafruit_WINC1500 WiFi(WINC_CS, WINC_IRQ, WINC_RST);

// Or just use hardware SPI (SCK/MOSI/MISO) and defaults, SS -> #10, INT -> #7, RST -> #5, EN -> 3-5V
//Adafruit_WINC1500 WiFi;

void setup() {
#ifdef WINC_EN
  pinMode(WINC_EN, OUTPUT);
  digitalWrite(WINC_EN, HIGH);
#endif

  // Initialize serial
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // Print a welcome message
  Serial.println("WINC1500 firmware check.");
  Serial.println();

  // Check for the presence of the shield
  Serial.print("WINC1500: ");
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("NOT PRESENT");
    return; // don't continue
  }
  Serial.println("DETECTED");

  // Print firmware version on the shield
  String fv = WiFi.firmwareVersion();
  Serial.print("Firmware version installed: ");
  Serial.println(fv);

  // Print required firmware version
  Serial.print("Firmware version required : ");
  Serial.println(WIFI_FIRMWARE_REQUIRED);

  // Check if the required version is installed
  Serial.println();
  if (fv == WIFI_FIRMWARE_REQUIRED) {
    Serial.println("Check result: PASSED");
  } else {
    Serial.println("Check result: NOT PASSED");
    Serial.println(" - The firmware version on the WINC1500 do not match the");
    Serial.println("   version required by the library, you may experience");
    Serial.println("   issues or failures.");
  }
}

void loop() {
  // do nothing
}
