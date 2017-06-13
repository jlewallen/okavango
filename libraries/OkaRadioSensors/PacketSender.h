#ifndef PACKET_SENDER_H_INCLUDED
#define PACKET_SENDER_H_INCLUDED

#include "Queue.h"
#include "network.h"

class PacketSender {
private:
    LoraRadio *radio = nullptr;
    Queue *queue = nullptr;

public:
    PacketSender(LoraRadio *radio, Queue *queue);

public:
    void send();

};

#endif
