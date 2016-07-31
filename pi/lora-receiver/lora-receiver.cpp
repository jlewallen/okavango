/*******************************************************************************
 *
 * Copyright (c) 2015 Thomas Telkamp
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 *******************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <pthread.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>

#include "rh_rf95.h"

#define PIN_SELECT      6
#define PIN_DIO_0       7
#define PIN_RESET       0
#define SPI_CHANNEL 0

typedef bool boolean;

const int32_t rssiCorrection = 157;
const sf_t sf = SF7;

typedef struct packet_t {
    int8_t size;
    char *data;
} packet_t;

void selectReceiver() {
    digitalWrite(PIN_SELECT, LOW);
}

void unselectReceiver() {
    digitalWrite(PIN_SELECT, HIGH);
}

int8_t readRegister(int8_t address) {
    unsigned char buffer[2];

    buffer[0] = address & 0x7F;
    buffer[1] = 0x00;

    selectReceiver();
    wiringPiSPIDataRW(SPI_CHANNEL, buffer, 2);
    unselectReceiver();

    return buffer[1];
}

void writeRegister(int8_t address, int8_t value) {
    unsigned char buffer[2];

    buffer[0] = address | 0x80;
    buffer[1] = value;

    selectReceiver();
    wiringPiSPIDataRW(SPI_CHANNEL, buffer, 2);
    unselectReceiver();
}

void printRegisters() {
    uint8_t registers[] = { 0x01, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
    0x11, 0x12, 0x13, 0x014, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x39 };

    for (uint8_t i = 0; i < sizeof(registers); i++) {
        printf("%x: %x\n", registers[i], readRegister(registers[i]));
    }
}

packet_t *readPacket() {
    int8_t irqFlags = readRegister(RH_RF95_REG_12_IRQ_FLAGS);

    if ((irqFlags & RH_RF95_PAYLOAD_CRC_ERROR_MASK) == RH_RF95_PAYLOAD_CRC_ERROR_MASK) {
        writeRegister(RH_RF95_REG_12_IRQ_FLAGS, 0xff);
        return NULL;
    } 

    if ((irqFlags & RH_RF95_RX_DONE_MASK) != RH_RF95_RX_DONE_MASK) {
        writeRegister(RH_RF95_REG_12_IRQ_FLAGS, 0xff);
        return NULL;
    } 

    int8_t currentAddress = readRegister(RH_RF95_REG_10_FIFO_RX_CURRENT_ADDR);
    int8_t receivedBytes = readRegister(RH_RF95_REG_13_RX_NB_BYTES);

    packet_t *pkt = (packet_t *)malloc(sizeof(packet_t) + receivedBytes + 1);
    pkt->size = receivedBytes;
    pkt->data = ((char *)pkt) + sizeof(packet_t);

    writeRegister(RH_RF95_REG_0D_FIFO_ADDR_PTR, currentAddress);

    for (int8_t i = 0; i < receivedBytes; i++) {
        pkt->data[i] = (char)readRegister(RH_RF95_REG_00_FIFO);
    }

    writeRegister(RH_RF95_REG_12_IRQ_FLAGS, 0xff); // clear all IRQ flags

    return pkt;
}

void resetRadio() {
    digitalWrite(PIN_RESET, LOW);
    delay(100);
    digitalWrite(PIN_RESET, HIGH);
    delay(100);
}

void detectChip() {
    int8_t version = readRegister(RH_RF95_REG_42_VERSION);
    if (version != 0x12) {
        fprintf(stderr, "Unrecognized transceiver\n");
        fprintf(stderr, "Version: 0x%x\n",version);
        exit(1);
    }
}

void setupRadio() {
    resetRadio();
    detectChip();

    writeRegister(RH_RF95_REG_01_OP_MODE, SX72_MODE_SLEEP);
    delay(10);

    uint32_t frf = (915.0 * 1000000.0) / RH_RF95_FSTEP;
    writeRegister(RH_RF95_REG_06_FRF_MSB, (uint8_t)((frf >> 16) & 0xff));
    writeRegister(RH_RF95_REG_07_FRF_MID, (uint8_t)((frf >> 8) & 0xff));
    writeRegister(RH_RF95_REG_08_FRF_LSB, (uint8_t)((frf) & 0xff));

    if (sf == SF10 || sf == SF11 || sf == SF12) {
        writeRegister(RH_RF95_REG_1F_SYMB_TIMEOUT_LSB, 0x05);
    } else {
        writeRegister(RH_RF95_REG_1F_SYMB_TIMEOUT_LSB, 0x08);
    }
    writeRegister(RH_RF95_REG_23_MAX_PAYLOAD_LENGTH, 0x80);
    writeRegister(RH_RF95_REG_22_PAYLOAD_LENGTH, PAYLOAD_LENGTH);
    writeRegister(RH_RF95_REG_24_HOP_PERIOD, 0xFF);

    writeRegister(RH_RF95_REG_0E_FIFO_TX_BASE_ADDR, 0);
    writeRegister(RH_RF95_REG_0F_FIFO_RX_BASE_ADDR, 0);

    writeRegister(RH_RF95_REG_0D_FIFO_ADDR_PTR, readRegister(RH_RF95_REG_0F_FIFO_RX_BASE_ADDR));

    writeRegister(RH_RF95_REG_0C_LNA, RH_RF95_LNA_LOW_GAIN);
    writeRegister(RH_RF95_REG_01_OP_MODE, SX72_MODE_RX_CONTINUOS);

    writeRegister(RH_RF95_REG_09_PA_CONFIG, 0x8f);
    writeRegister(RH_RF95_REG_1F_SYMB_TIMEOUT_LSB, 0x64);
    writeRegister(RH_RF95_REG_22_PAYLOAD_LENGTH, 0x01);
    writeRegister(RH_RF95_REG_23_MAX_PAYLOAD_LENGTH, 0xff);
    writeRegister(RH_RF95_REG_24_HOP_PERIOD, 0x0);
    writeRegister(RH_RF95_REG_26_MODEM_CONFIG3, 0x0);

    #if 0
    printRegisters();
    #endif
}

int32_t getSnr() {
    int8_t value = readRegister(RH_RF95_REG_19_PKT_SNR_VALUE);
    if (value & 0x80) {
        value = ((~value + 1) & 0xFF) >> 2;
        return -value;
    }
    else {
        return (value & 0xFF) >> 2;
    }
}

int32_t getPacketRssi() {
    return readRegister(RH_RF95_REG_1A_PKT_RSSI_VALUE) - rssiCorrection;
}

int32_t getRssi() {
    return readRegister(RH_RF95_REG_1B_RSSI_VALUE) - rssiCorrection;
}

void receivePacket() {
    packet_t *pkt = readPacket();
    if (pkt != NULL) {
        int8_t headerTo = pkt->data[0];
        int8_t headerFrom = pkt->data[1];
        int8_t headerId = pkt->data[2];
        int8_t headerFlags = pkt->data[3];

        printf("Packet RSSI: %d, ", getPacketRssi());
        printf("RSSI: %d, ", getRssi());
        printf("SNR: %d, ", getSnr());
        printf("Length: %d, ", (int32_t)pkt->size);
        printf("To: %x, From: %x, Id: %x, Flags: %x", headerTo, headerFrom, headerId, headerFlags);
        printf("\n");

        pkt->data[pkt->size] = 0;
        printf("%s\n", pkt->data + 4);

        fflush(stdout);

        free(pkt);
        pkt = NULL;
    }
}

void handleIsr() {
    receivePacket();
}

int32_t main() {
    // struct timeval nowtime;
    // uint32_t lastSeconds;

    wiringPiSetup();

    pinMode(PIN_SELECT, OUTPUT);
    pinMode(PIN_DIO_0, INPUT);
    pinMode(PIN_RESET, OUTPUT);

    wiringPiSPISetup(SPI_CHANNEL, 500000);

    setupRadio();

    wiringPiISR(PIN_DIO_0, INT_EDGE_RISING, handleIsr);

    while (true) {
        /*
        gettimeofday(&nowtime, NULL);
        uint32_t nowSeconds = (uint32_t)(nowtime.tv_sec);
        if (nowSeconds - lastSeconds >= 5) {
            lastSeconds = nowSeconds;
        }
        */
        delay(500);
    }

    return 0;

}

