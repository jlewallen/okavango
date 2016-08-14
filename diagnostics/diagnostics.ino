#include "Platforms.h"
#include "core.h"
#include "Repl.h"
#include "LoraRadio.h"

class Sniffer {
private:
    LoraRadio *radio;
    bool enabled;

public:
    Sniffer(LoraRadio *radio) : radio(radio), enabled(true) {
    }

    void setEnabled(bool enabled) {
        this->enabled = enabled;
    }

    void tick() {
        if (enabled) {
            radio->tick();

            if (radio->hasPacket()) {
                log((fk_network_packet_t *)radio->getPacket(), radio->getPacketSize());
                radio->clear();
            }
        }
    }

private:
    void log(fk_network_packet_t *packet, size_t packetSize) {
        Serial.println();

        switch (packet->kind) {
        case FK_PACKET_KIND_PING: {
            Serial.println("PING");
            break;
        }
        case FK_PACKET_KIND_PONG: {
            Serial.println("PONG");
            break;
        }
        case FK_PACKET_KIND_ACK: {
            Serial.println("ACK");
            break;
        }
        case FK_PACKET_KIND_ATLAS_SENSORS: {
            Serial.println("ATLAS_SENSORS");
            break;
        }
        case FK_PACKET_KIND_WEATHER_STATION: {
            Serial.println("WEATHER_STATION");
            break;
        }
        case FK_PACKET_KIND_DATA_BOAT_SENSORS: {
            Serial.println("DATA_BOAT_SENSORS");
            break;
        }
        }
    }
};

class DiagnosticsRepl : public Repl {
private:
    Sniffer *sniffer;

public:
    DiagnosticsRepl(Sniffer *sniffer) : sniffer(sniffer) {
    }

    bool doWork() {
        return false;
    }
    
    void handle(String command) {
        if (command == "ping") {

        }
    }
};

CorePlatform corePlatform;

void setup() {
    Serial.begin(115200);

    #ifdef WAIT_FOR_SERIAL
    while (!Serial) {
        delay(100);
        if (millis() > WAIT_FOR_SERIAL) {
            break;
        }
    }
    #endif

    Serial.println("Begin");

    corePlatform.setup();

    Serial.println("Loop");
}

void loop() {
    LoraRadio radio(PIN_RFM95_CS, PIN_RFM95_INT, PIN_RFM95_RST);
    Sniffer sniffer(&radio);
    DiagnosticsRepl repl(&sniffer);

    while (1) {
        repl.tick();

        sniffer.tick();

        delay(50);
    }
}

// vim: set ft=cpp:

