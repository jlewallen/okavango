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

typedef struct radio_t {
    pthread_mutex_t mutex;
    uint8_t mode;
} radio_t;

radio_t global_radio;

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

void radio_set_frequency(float centre) {
    uint32_t frf = (centre * 1000000.0) / RH_RF95_FSTEP;
    spi_write_register(RH_RF95_REG_06_FRF_MSB, (uint8_t)((frf >> 16) & 0xff));
    spi_write_register(RH_RF95_REG_07_FRF_MID, (uint8_t)((frf >> 8) & 0xff));
    spi_write_register(RH_RF95_REG_08_FRF_LSB, (uint8_t)((frf) & 0xff));
}

typedef struct modem_config_t {
    uint8_t reg_1d;
    uint8_t reg_1e;
    uint8_t reg_26;
} modem_config_t;

modem_config_t Bw125Cr45Sf128 = { 0x72, 0x74, 0x00};

void radio_set_modem_config(modem_config_t *config) {
    spi_write_register(RH_RF95_REG_1D_MODEM_CONFIG1, config->reg_1d);
    spi_write_register(RH_RF95_REG_1E_MODEM_CONFIG2, config->reg_1e);
    spi_write_register(RH_RF95_REG_26_MODEM_CONFIG3, config->reg_26);
}

void radio_set_tx_power(int8_t power) {
    if (power > 20)
        power = 20;
    if (power < 5)
        power = 5;
    spi_write_register(RH_RF95_REG_09_PA_CONFIG, RH_RF95_PA_SELECT | (power - 5));
}

bool radio_setup() {
    radio_reset();

    if (!radio_detect_chip()) {
        return false;
    }

    spi_write_register(RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_SLEEP | RH_RF95_LONG_RANGE_MODE);
    delay(10);

    spi_write_register(RH_RF95_REG_0D_FIFO_ADDR_PTR, spi_read_register(RH_RF95_REG_0F_FIFO_RX_BASE_ADDR));
    spi_write_register(RH_RF95_REG_23_MAX_PAYLOAD_LENGTH, 0x80);
    spi_write_register(RH_RF95_REG_0E_FIFO_TX_BASE_ADDR, 0);
    spi_write_register(RH_RF95_REG_0F_FIFO_RX_BASE_ADDR, 0);

    radio_set_mode_idle();

    radio_set_modem_config(&Bw125Cr45Sf128); 
    radio_set_frequency(915.0f);
    radio_set_preamble_length(8);
    radio_set_tx_power(13);

    #if 0
    radio_print_registers();
    #endif

    return true;
}

void radio_print_registers() {
    uint8_t registers[] = { 0x01, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
    0x11, 0x12, 0x13, 0x014, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x39 };

    for (uint8_t i = 0; i < sizeof(registers); i++) {
        printf("%x: %x\n", registers[i], spi_read_register(registers[i]));
    }
}

uint8_t radio_get_mode() {
    return spi_read_register(RH_RF95_REG_01_OP_MODE);
}

void radio_set_mode_tx() {
    if (global_radio.mode != RH_RF95_MODE_TX) {
        spi_write_register(RH_RF95_REG_40_DIO_MAPPING1, 0x40); // IRQ on TxDone
        spi_write_register(RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_TX);
        global_radio.mode = RH_RF95_MODE_TX;
    }
}

void radio_set_mode_rx() {
    if (global_radio.mode != RH_RF95_MODE_RXCONTINUOUS) {
        spi_write_register(RH_RF95_REG_40_DIO_MAPPING1, 0x00); // IRQ on RxDone
        spi_write_register(RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_RXCONTINUOUS);
        global_radio.mode = RH_RF95_MODE_RXCONTINUOUS;
    }
}

void radio_set_mode_idle() {
    if (global_radio.mode != RH_RF95_MODE_STDBY) {
        spi_write_register(RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_STDBY);
        global_radio.mode = RH_RF95_MODE_STDBY;
    }
}

void radio_send_packet(lora_packet_t *packet) {
    radio_set_mode_idle();

    spi_write_register(RH_RF95_REG_0E_FIFO_TX_BASE_ADDR, 0);
    spi_write_register(RH_RF95_REG_0D_FIFO_ADDR_PTR, 0);

    spi_write_register(RH_RF95_REG_00_FIFO, packet->to);
    spi_write_register(RH_RF95_REG_00_FIFO, packet->from);
    spi_write_register(RH_RF95_REG_00_FIFO, packet->id);
    spi_write_register(RH_RF95_REG_00_FIFO, packet->flags);

    for (uint8_t i = 0; i < packet->size; ++i) {
        spi_write_register(RH_RF95_REG_00_FIFO, packet->data[i]);
    }

    spi_write_register(RH_RF95_REG_22_PAYLOAD_LENGTH, packet->size + SX1272_HEADER_LENGTH);

    radio_set_mode_tx();
}

raw_packet_t *radio_read_raw_packet() {
    uint8_t current_address = spi_read_register(RH_RF95_REG_10_FIFO_RX_CURRENT_ADDR);
    uint8_t received_bytes = spi_read_register(RH_RF95_REG_13_RX_NB_BYTES);

    raw_packet_t *pkt = (raw_packet_t *)malloc(sizeof(raw_packet_t) + received_bytes + 1);
    pkt->size = received_bytes;
    pkt->data = ((uint8_t *)pkt) + sizeof(raw_packet_t);

    spi_write_register(RH_RF95_REG_0D_FIFO_ADDR_PTR, current_address);

    for (uint8_t i = 0; i < received_bytes; i++) {
        pkt->data[i] = (char)spi_read_register(RH_RF95_REG_00_FIFO);
    }

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

void radio_set_preamble_length(uint16_t length) {
    spi_write_register(RH_RF95_REG_20_PREAMBLE_MSB, length >> 8);
    spi_write_register(RH_RF95_REG_21_PREAMBLE_LSB, length & 0xff);
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

#define RH_RF95_RSSI_CORRECTION 157

int32_t radio_get_packet_rssi() {
    return spi_read_register(RH_RF95_REG_1A_PKT_RSSI_VALUE) - RH_RF95_RSSI_CORRECTION;
}

int32_t radio_get_rssi() {
    return spi_read_register(RH_RF95_REG_1B_RSSI_VALUE) - RH_RF95_RSSI_CORRECTION;
}

lora_packet_t *lora_packet_new(size_t size) {
    lora_packet_t *lora = (lora_packet_t *)malloc(sizeof(lora_packet_t) + size);
    memset((void *)lora, 0, sizeof(lora_packet_t) + size);

    lora->size = size;
    lora->data = ((uint8_t *)lora) + sizeof(lora_packet_t);
    lora->to = 0xff;
    lora->from = 0xff;

    return lora;
}

lora_packet_t *lora_packet_create_from(raw_packet_t *raw) {
    if (raw->size <= SX1272_HEADER_LENGTH) {
        return NULL;
    }

    lora_packet_t *lora = (lora_packet_t *)malloc(sizeof(lora_packet_t));
    lora->size = raw->size - SX1272_HEADER_LENGTH;
    lora->data = ((uint8_t *)raw->data) + SX1272_HEADER_LENGTH;
    lora->to = raw->data[0];
    lora->from = raw->data[1];
    lora->id = raw->data[2];
    lora->flags = raw->data[3];

    return lora;
}

time_t last_packet_at = 0;

void receive_packet() {
    raw_packet_t *raw_packet = radio_read_raw_packet();
    if (raw_packet != NULL) {
        lora_packet_t *lora_packet = lora_packet_create_from(raw_packet);
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

                lora_packet_t *ack = lora_packet_new(1);
                radio_send_packet(ack);
                free(ack);

                last_packet_at = time(NULL);
            }

            free(lora_packet);
            lora_packet = NULL;
        }

        free(raw_packet);
        raw_packet = NULL;
    }
}

void handle_isr() {
    pthread_mutex_lock(&global_radio.mutex);

    uint8_t flags = spi_read_register(RH_RF95_REG_12_IRQ_FLAGS);

    if ((flags & RH_RF95_PAYLOAD_CRC_ERROR_MASK) == RH_RF95_PAYLOAD_CRC_ERROR_MASK) {
        // Oh no
    } 
    else if ((flags & RH_RF95_RX_DONE) == RH_RF95_RX_DONE) {
        receive_packet();
        radio_set_mode_idle();
    }
    else if ((flags & RH_RF95_TX_DONE) == RH_RF95_TX_DONE) {
        radio_set_mode_idle();
    }

    spi_write_register(RH_RF95_REG_12_IRQ_FLAGS, 0xff); // clear all IRQ flags

    pthread_mutex_unlock(&global_radio.mutex);
}

int32_t main() {
    wiringPiSetup();

    pinMode(PIN_SELECT, OUTPUT);
    pinMode(PIN_DIO_0, INPUT);
    pinMode(PIN_RESET, OUTPUT);

    wiringPiSPISetup(SPI_CHANNEL, 500000);
    wiringPiISR(PIN_DIO_0, INT_EDGE_RISING, handle_isr);

    pthread_mutex_init(&global_radio.mutex, NULL);

    time_t check_radio_every = 1000;
    time_t radio_checked_at = 0;
    bool have_radio = false;
    while (true) {
        pthread_mutex_lock(&global_radio.mutex);

        if (!have_radio) {
            if (radio_setup()) {
                radio_set_mode_rx();
                have_radio = true;
            }
        }
        else {
            if (time(NULL) - radio_checked_at > check_radio_every) {
                have_radio = radio_detect_chip();
                radio_checked_at = time(NULL);

                if (time(NULL) - last_packet_at > 60) {
                    fprintf(stderr, "Last packet received %d seconds ago\n", (int32_t)(time(NULL) - last_packet_at));
                }
            }

            if (global_radio.mode == RH_RF95_MODE_STDBY) {
                radio_set_mode_rx();
            }
        }

        pthread_mutex_unlock(&global_radio.mutex);

        delay(100);
    }

    pthread_mutex_destroy(&global_radio.mutex);

    return 0;

}

