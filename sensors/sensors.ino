#include "Platforms.h"
#include "AtlasScientific.h"
#include "SerialPortExpander.h"
#include "LoraRadio.h"

SerialPortExpander portExpander(PORT_EXPANDER_SELECT_PIN_0, PORT_EXPANDER_SELECT_PIN_1, PORT_EXPANDER_SELECT_PIN_2);
LoraRadio radio(RFM95_CS, RFM95_INT, RFM95_RST);
AtlasScientificBoard board;

void finish() {
    portExpander.select(3);

    while (true) {
        delay(1000);
    }
}

#define NUMBER_OF_VALUES 7

typedef struct sensors_packet_t {
    uint8_t kind;
    uint32_t time;
    float values[NUMBER_OF_VALUES];
} sensors_packet_t;

uint8_t valueIndex = 0;
sensors_packet_t packet;

void populatePacket() {
    for (uint8_t i = 0; i < board.getNumberOfValues(); ++i) {
        if (valueIndex < NUMBER_OF_VALUES) {
            packet.values[valueIndex++] = board.getValues()[i];
        }
        else {
            Serial.println("Not enough room for values.");
        }
    }
}

void setup() {
    Serial.begin(115200);
    #ifdef WAIT_FOR_SERIAL
    while (!Serial) {
        delay(100);
        if (millis() > WAIT_FOR_SERIAL) {
            break;
        }
    }
    #else
    delay(3000);
    #endif

    Serial.println("Begin");

    portExpander.setup();
    portExpander.select(0);

    board.setSerial(&portExpanderSerial);
    board.start();

    radio.setup();

    platformPostSetup();

    memset((void *)&packet, 0, sizeof(sensors_packet_t));

    Serial.println("Loop");
}

void loop() {
    board.tick();
    delay(50);

    if (board.isDone()) {
        populatePacket();
        byte newPort = portExpander.getPort() + 1;
        portExpander.select(newPort);
        if (newPort < 3) {
            Serial.println("Next sensor");
            board.start();
        }
        else if (newPort == 3) {
            Serial.println("Conductivity");
            board.setSerial(&conductivitySerial);
            board.start(OPEN_CONDUCTIVITY_SERIAL_ON_START);
        }
        else {
            Serial.println("Done");
            radio.send((uint8_t *)&packet, sizeof(sensors_packet_t));
            delay(5000);
            radio.sleep();
            finish();
        }
    }
}

// vim: set ft=cpp:
