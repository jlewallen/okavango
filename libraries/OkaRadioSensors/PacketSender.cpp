#include <Adafruit_SleepyDog.h>

#include "PacketSender.h"

PacketSender::PacketSender(LoraRadio *radio, IQueue *queue) :
    radio(radio), queue(queue) {
}

void PacketSender::send() {
    NetworkProtocolState networkProtocol(FK_IDENTITY_ATLAS, NetworkState::PingForListener, radio, queue, NULL);

    Watchdog.enable();

    if (radio->setup()) {
        DEBUG_PRINTLN("Radio ready");

        DEBUG_PRINT("Queue: ");
        DEBUG_PRINTLN(queue->size());

        while (true) {
            Watchdog.reset();

            networkProtocol.tick();

            if (networkProtocol.isQueueEmpty() || networkProtocol.isNobodyListening()) {
                break;
            }

            delay(10);
        }

        radio->sleep();
    }
    else {
        DEBUG_PRINTLN("No radio available");
    }

    Watchdog.disable();
}
