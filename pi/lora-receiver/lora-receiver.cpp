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

const int32_t rssi_correction = 157;
const sf_t sf = SF7;

typedef struct raw_packet_t {
    uint8_t size;
    uint8_t *data;
} raw_packet_t;

typedef struct lora_packet_t {
    uint8_t size;
    uint8_t *data;
    uint8_t to;
    uint8_t from;
    uint8_t id;
    uint8_t flags;
} lora_packet_t;

#define SENSORS_PACKET_NUMBER_VALUES 10

typedef struct sensors_packet_t {
    uint8_t kind;
    uint32_t time;
    float battery;
    float values[SENSORS_PACKET_NUMBER_VALUES];
} sensors_packet_t;

void spi_select_radio() {
    digitalWrite(PIN_SELECT, LOW);
}

void spi_unselect_radio() {
    digitalWrite(PIN_SELECT, HIGH);
}

int8_t spi_read_register(int8_t address) {
    unsigned char buffer[2];

    buffer[0] = address & 0x7F;
    buffer[1] = 0x00;

    spi_select_radio();
    wiringPiSPIDataRW(SPI_CHANNEL, buffer, 2);
    spi_unselect_radio();

    return buffer[1];
}

void spi_write_register(int8_t address, int8_t value) {
    unsigned char buffer[2];

    buffer[0] = address | 0x80;
    buffer[1] = value;

    spi_select_radio();
    wiringPiSPIDataRW(SPI_CHANNEL, buffer, 2);
    spi_unselect_radio();
}

void radio_print_registers() {
    uint8_t registers[] = { 0x01, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
    0x11, 0x12, 0x13, 0x014, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x39 };

    for (uint8_t i = 0; i < sizeof(registers); i++) {
        printf("%x: %x\n", registers[i], spi_read_register(registers[i]));
    }
}

raw_packet_t *radio_read_raw_packet() {
    uint8_t flags = spi_read_register(RH_RF95_REG_12_IRQ_FLAGS);

    if ((flags & RH_RF95_PAYLOAD_CRC_ERROR_MASK) == RH_RF95_PAYLOAD_CRC_ERROR_MASK) {
        spi_write_register(RH_RF95_REG_12_IRQ_FLAGS, 0xff);
        return NULL;
    } 

    if ((flags & RH_RF95_RX_DONE_MASK) != RH_RF95_RX_DONE_MASK) {
        spi_write_register(RH_RF95_REG_12_IRQ_FLAGS, 0xff);
        return NULL;
    } 

    uint8_t current_address = spi_read_register(RH_RF95_REG_10_FIFO_RX_CURRENT_ADDR);
    uint8_t received_bytes = spi_read_register(RH_RF95_REG_13_RX_NB_BYTES);

    raw_packet_t *pkt = (raw_packet_t *)malloc(sizeof(raw_packet_t) + received_bytes + 1);
    pkt->size = received_bytes;
    pkt->data = ((uint8_t *)pkt) + sizeof(raw_packet_t);

    spi_write_register(RH_RF95_REG_0D_FIFO_ADDR_PTR, current_address);

    for (uint8_t i = 0; i < received_bytes; i++) {
        pkt->data[i] = (char)spi_read_register(RH_RF95_REG_00_FIFO);
    }

    spi_write_register(RH_RF95_REG_12_IRQ_FLAGS, 0xff); // clear all IRQ flags

    return pkt;
}

void radio_reset() {
    digitalWrite(PIN_RESET, LOW);
    delay(100);
    digitalWrite(PIN_RESET, HIGH);
    delay(100);
}

bool radio_detect_chip() {
    uint8_t version = spi_read_register(RH_RF95_REG_42_VERSION);
    if (version == 0) {
        fprintf(stderr, "No radio found\n");
        return false;
    }
    if (version != 0x12) {
        fprintf(stderr, "Unrecognized transceiver (Version: 0x%x)\n", version);
        return false;
    }
    return true;
}

bool radio_setup() {
    radio_reset();

    if (!radio_detect_chip()) {
        return false;
    }

    spi_write_register(RH_RF95_REG_01_OP_MODE, SX72_MODE_SLEEP);
    delay(10);

    uint32_t frf = (915.0 * 1000000.0) / RH_RF95_FSTEP;
    spi_write_register(RH_RF95_REG_06_FRF_MSB, (uint8_t)((frf >> 16) & 0xff));
    spi_write_register(RH_RF95_REG_07_FRF_MID, (uint8_t)((frf >> 8) & 0xff));
    spi_write_register(RH_RF95_REG_08_FRF_LSB, (uint8_t)((frf) & 0xff));

    if (sf == SF10 || sf == SF11 || sf == SF12) {
        spi_write_register(RH_RF95_REG_1F_SYMB_TIMEOUT_LSB, 0x05);
    } else {
        spi_write_register(RH_RF95_REG_1F_SYMB_TIMEOUT_LSB, 0x08);
    }
    spi_write_register(RH_RF95_REG_23_MAX_PAYLOAD_LENGTH, 0x80);
    spi_write_register(RH_RF95_REG_22_PAYLOAD_LENGTH, PAYLOAD_LENGTH);
    spi_write_register(RH_RF95_REG_24_HOP_PERIOD, 0xFF);

    spi_write_register(RH_RF95_REG_0E_FIFO_TX_BASE_ADDR, 0);
    spi_write_register(RH_RF95_REG_0F_FIFO_RX_BASE_ADDR, 0);

    spi_write_register(RH_RF95_REG_0D_FIFO_ADDR_PTR, spi_read_register(RH_RF95_REG_0F_FIFO_RX_BASE_ADDR));

    spi_write_register(RH_RF95_REG_0C_LNA, RH_RF95_LNA_LOW_GAIN);
    spi_write_register(RH_RF95_REG_01_OP_MODE, SX72_MODE_RX_CONTINUOS);

    spi_write_register(RH_RF95_REG_09_PA_CONFIG, 0x8f);
    spi_write_register(RH_RF95_REG_1F_SYMB_TIMEOUT_LSB, 0x64);
    spi_write_register(RH_RF95_REG_22_PAYLOAD_LENGTH, 0x01);
    spi_write_register(RH_RF95_REG_23_MAX_PAYLOAD_LENGTH, 0xff);
    spi_write_register(RH_RF95_REG_24_HOP_PERIOD, 0x0);
    spi_write_register(RH_RF95_REG_26_MODEM_CONFIG3, 0x0);

    #if 0
    radio_print_registers();
    #endif

    return true;
}

int32_t radio_get_snr() {
    uint8_t value = spi_read_register(RH_RF95_REG_19_PKT_SNR_VALUE);
    if (value & 0x80) {
        value = ((~value + 1) & 0xFF) >> 2;
        return -value;
    }
    else {
        return (value & 0xFF) >> 2;
    }
}

int32_t radio_get_packet_rssi() {
    return spi_read_register(RH_RF95_REG_1A_PKT_RSSI_VALUE) - rssi_correction;
}

int32_t radio_get_rssi() {
    return spi_read_register(RH_RF95_REG_1B_RSSI_VALUE) - rssi_correction;
}

lora_packet_t *create_lora_packet(raw_packet_t *raw) {
    if (raw->size <= 4) {
        return NULL;
    }

    lora_packet_t *lora = (lora_packet_t *)malloc(sizeof(lora_packet_t));
    lora->size = raw->size - 4;
    lora->data = ((uint8_t *)raw->data) + 4;
    lora->to = raw->data[0];
    lora->from = raw->data[1];
    lora->id = raw->data[2];
    lora->flags = raw->data[3];

    return lora;
}

void receive_packet() {
    raw_packet_t *raw_packet = radio_read_raw_packet();
    if (raw_packet != NULL) {
        lora_packet_t *lora_packet = create_lora_packet(raw_packet);
        if (lora_packet != NULL) {
            printf("Packet RSSI: %d, ", radio_get_packet_rssi());
            printf("RSSI: %d, ", radio_get_rssi());
            printf("SNR: %d, ", radio_get_snr());
            printf("Length: %d, ", (int32_t)lora_packet->size);
            printf("To: %x, From: %x, Id: %x, Flags: %x", lora_packet->to, lora_packet->from, lora_packet->id, lora_packet->flags);
            printf("\n");

            uint8_t *frame = lora_packet->data;
            if (frame[0] == 0) {
                sensors_packet_t *sensors_packet = (sensors_packet_t *)frame;
                printf("%d %d %f", sensors_packet->kind, sensors_packet->time, sensors_packet->battery);
                for (int8_t i = 0; i < SENSORS_PACKET_NUMBER_VALUES; ++i) {
                    printf(" %f", sensors_packet->values[i]);
                }
                printf("\n");
            }

            free(lora_packet);
            lora_packet = NULL;
        }

        free(raw_packet);
        raw_packet = NULL;
    }
}

void handle_isr() {
    receive_packet();
}

int32_t main() {
    wiringPiSetup();

    pinMode(PIN_SELECT, OUTPUT);
    pinMode(PIN_DIO_0, INPUT);
    pinMode(PIN_RESET, OUTPUT);

    wiringPiSPISetup(SPI_CHANNEL, 500000);
    wiringPiISR(PIN_DIO_0, INT_EDGE_RISING, handle_isr);

    bool have_radio = false;
    while (true) {
        if (!have_radio) {
            if (radio_setup()) {
                have_radio = true;
            }
        }
        else {
            have_radio = radio_detect_chip();
        }

        delay(1000);
    }

    return 0;

}

