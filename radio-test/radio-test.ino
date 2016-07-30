#include <SPI.h>
#include <RH_RF95.h>

#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2

#define RF95_FREQ 915.0

#define RECEIVER

RH_RF95 rf95(RFM95_CS, RFM95_INT);

void setup() {
    pinMode(RFM95_RST, OUTPUT);
    digitalWrite(RFM95_RST, HIGH);

    while (!Serial) {
        delay(100);
    }
    Serial.begin(115200);
    delay(100);

    Serial.println("Arduino LoRa TX Test!");

    digitalWrite(RFM95_RST, LOW);
    delay(10);
    digitalWrite(RFM95_RST, HIGH);
    delay(10);

    while (!rf95.init()) {
        Serial.println("LoRa radio init failed");
        while (true) {
        }
    }
    Serial.println("LoRa radio init OK!");

    if (!rf95.setFrequency(RF95_FREQ)) {
        Serial.println("setFrequency failed");
        while (true) {
        }
    }
    Serial.print("Set Freq to: ");
    Serial.println(RF95_FREQ);

    rf95.setTxPower(23, false);
}

int16_t packetNumber = 0;

void transmitter() {
    Serial.println("Sending to rf95_server");

    char radiopacket[20] = "Hello World #      ";
    itoa(packetNumber++, radiopacket + 13, 10);

    Serial.print("Sending ");
    Serial.println(radiopacket);
    radiopacket[19] = 0;

    Serial.println("Sending...");
    delay(10);
    rf95.send((uint8_t *)radiopacket, 20);

    Serial.println("Waiting for packet to complete...");
    delay(10);
    rf95.waitPacketSent();

    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    Serial.println("Waiting for reply...");
    delay(10);
    if (rf95.waitAvailableTimeout(1000))
    { 
        if (rf95.recv(buf, &len))
        {
            Serial.print("Got reply: ");
            Serial.println((char*)buf);
            Serial.print("RSSI: ");
            Serial.println(rf95.lastRssi(), DEC);    
        }
        else
        {
            Serial.println("Receive failed");
        }
    }
    else
    {
        Serial.println("No reply, is there a listener around?");
    }

    delay(1000);
}

void receiver() {
    if (rf95.available())
    {
        uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);

        if (rf95.recv(buf, &len))
        {
            RH_RF95::printBuffer("Received: ", buf, len);
            Serial.print("Got: ");
            Serial.println((char*)buf);
            Serial.print("RSSI: ");
            Serial.println(rf95.lastRssi(), DEC);

            uint8_t data[] = "And hello back to you";
            rf95.send(data, sizeof(data));
            rf95.waitPacketSent();
            Serial.println("Sent a reply");
        }
        else
        {
            Serial.println("Receive failed");
        }
    }
}

void loop() {
#ifdef RECEIVER
    receiver();
#else
    transmitter();
#endif
}

// vim: set ft=cpp:
