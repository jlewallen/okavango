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

#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <cstdlib>
#include <sys/time.h>
#include <cstring>

#include <sys/ioctl.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>

typedef bool boolean;

int8_t currentMode = 0x81;
int8_t receivedBytes;
char message[256];
bool sx1272 = true;

enum sf_t { SF7 = 7, SF8, SF9, SF10, SF11, SF12 };

int8_t pinSelect = 6;
int8_t pinDio0 = 7;
int8_t pinReset = 0;
sf_t sf = SF7;

#define RH_RF95_REG_00_FIFO                                0x00
#define RH_RF95_REG_01_OP_MODE                             0x01
#define RH_RF95_REG_02_RESERVED                            0x02
#define RH_RF95_REG_03_RESERVED                            0x03
#define RH_RF95_REG_04_RESERVED                            0x04
#define RH_RF95_REG_05_RESERVED                            0x05
#define RH_RF95_REG_06_FRF_MSB                             0x06
#define RH_RF95_REG_07_FRF_MID                             0x07
#define RH_RF95_REG_08_FRF_LSB                             0x08
#define RH_RF95_REG_09_PA_CONFIG                           0x09
#define RH_RF95_REG_0A_PA_RAMP                             0x0a
#define RH_RF95_REG_0B_OCP                                 0x0b
#define RH_RF95_REG_0C_LNA                                 0x0c
#define RH_RF95_REG_0D_FIFO_ADDR_PTR                       0x0d
#define RH_RF95_REG_0E_FIFO_TX_BASE_ADDR                   0x0e
#define RH_RF95_REG_0F_FIFO_RX_BASE_ADDR                   0x0f
#define RH_RF95_REG_10_FIFO_RX_CURRENT_ADDR                0x10
#define RH_RF95_REG_11_IRQ_FLAGS_MASK                      0x11
#define RH_RF95_REG_12_IRQ_FLAGS                           0x12
#define RH_RF95_REG_13_RX_NB_BYTES                         0x13
#define RH_RF95_REG_14_RX_HEADER_CNT_VALUE_MSB             0x14
#define RH_RF95_REG_15_RX_HEADER_CNT_VALUE_LSB             0x15
#define RH_RF95_REG_16_RX_PACKET_CNT_VALUE_MSB             0x16
#define RH_RF95_REG_17_RX_PACKET_CNT_VALUE_LSB             0x17
#define RH_RF95_REG_18_MODEM_STAT                          0x18
#define RH_RF95_REG_19_PKT_SNR_VALUE                       0x19
#define RH_RF95_REG_1A_PKT_RSSI_VALUE                      0x1a
#define RH_RF95_REG_1B_RSSI_VALUE                          0x1b
#define RH_RF95_REG_1C_HOP_CHANNEL                         0x1c
#define RH_RF95_REG_1D_MODEM_CONFIG1                       0x1d
#define RH_RF95_REG_1E_MODEM_CONFIG2                       0x1e
#define RH_RF95_REG_1F_SYMB_TIMEOUT_LSB                    0x1f
#define RH_RF95_REG_20_PREAMBLE_MSB                        0x20
#define RH_RF95_REG_21_PREAMBLE_LSB                        0x21
#define RH_RF95_REG_22_PAYLOAD_LENGTH                      0x22
#define RH_RF95_REG_23_MAX_PAYLOAD_LENGTH                  0x23
#define RH_RF95_REG_24_HOP_PERIOD                          0x24
#define RH_RF95_REG_25_FIFO_RX_BYTE_ADDR                   0x25
#define RH_RF95_REG_26_MODEM_CONFIG3                       0x26

#define RH_RF95_REG_39_SYNC_WORD                           0x39
#define RH_RF95_REG_40_DIO_MAPPING1                        0x40
#define RH_RF95_REG_41_DIO_MAPPING2                        0x41
#define RH_RF95_REG_42_VERSION                             0x42

#define RH_RF95_REG_4B_TCXO                                0x4b
#define RH_RF95_REG_4D_PA_DAC                              0x4d
#define RH_RF95_REG_5B_FORMER_TEMP                         0x5b
#define RH_RF95_REG_61_AGC_REF                             0x61
#define RH_RF95_REG_62_AGC_THRESH1                         0x62
#define RH_RF95_REG_63_AGC_THRESH2                         0x63
#define RH_RF95_REG_64_AGC_THRESH3                         0x64

#define SX72_MODE_RX_CONTINUOS      0x85
#define SX72_MODE_TX                0x83
#define SX72_MODE_SLEEP             0x80
#define SX72_MODE_STANDBY           0x81

#define PAYLOAD_LENGTH              0x40

#define RH_RF95_LNA_MAX_GAIN                0x23
#define RH_RF95_LNA_OFF_GAIN                0x00
#define RH_RF95_LNA_LOW_GAIN		    	0x20

// RH_RF95_REG_0C_LNA                                 0x0c
#define RH_RF95_LNA_GAIN                              0xe0
#define RH_RF95_LNA_BOOST                             0x03
#define RH_RF95_LNA_BOOST_DEFAULT                     0x00
#define RH_RF95_LNA_BOOST_150PC                       0x11

// RH_RF95_REG_11_IRQ_FLAGS_MASK                      0x11
#define RH_RF95_RX_TIMEOUT_MASK                       0x80
#define RH_RF95_RX_DONE_MASK                          0x40
#define RH_RF95_PAYLOAD_CRC_ERROR_MASK                0x20
#define RH_RF95_VALID_HEADER_MASK                     0x10
#define RH_RF95_TX_DONE_MASK                          0x08
#define RH_RF95_CAD_DONE_MASK                         0x04
#define RH_RF95_FHSS_CHANGE_CHANNEL_MASK              0x02
#define RH_RF95_CAD_DETECTED_MASK                     0x01

// RH_RF95_REG_12_IRQ_FLAGS                           0x12
#define RH_RF95_RX_TIMEOUT                            0x80
#define RH_RF95_RX_DONE                               0x40
#define RH_RF95_PAYLOAD_CRC_ERROR                     0x20
#define RH_RF95_VALID_HEADER                          0x10
#define RH_RF95_TX_DONE                               0x08
#define RH_RF95_CAD_DONE                              0x04
#define RH_RF95_FHSS_CHANGE_CHANNEL                   0x02
#define RH_RF95_CAD_DETECTED                          0x01


#define SX72_MC2_FSK                0x00
#define SX72_MC2_SF7                0x70
#define SX72_MC2_SF8                0x80
#define SX72_MC2_SF9                0x90
#define SX72_MC2_SF10               0xA0
#define SX72_MC2_SF11               0xB0
#define SX72_MC2_SF12               0xC0

#define SX72_MC1_LOW_DATA_RATE_OPTIMIZE  0x01 // mandated for SF11 and SF12

// set frequency
// uint64_t frf = ((uint64_t)freq << 19) / 32000000;
// The crystal oscillator frequency of the module
#define RH_RF95_FXOSC 32000000.0

// The Frequency Synthesizer step = RH_RF95_FXOSC / 2^^19
#define RH_RF95_FSTEP  (RH_RF95_FXOSC / 524288)

#define SPI_CHANNEL 0

void selectReceiver() {
    digitalWrite(pinSelect, LOW);
}

void unselectReceiver() {
    digitalWrite(pinSelect, HIGH);
}

int8_t readRegister(int8_t address) {
    unsigned char buffer[2];

    selectReceiver();
    buffer[0] = address & 0x7F;
    buffer[1] = 0x00;
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

    uint8_t i;
    for (i = 0; i < sizeof(registers); i++) {
        printf("%x: %x\n", registers[i], readRegister(registers[i]));
    }
}

boolean receivePkt(char *payload)
{
    writeRegister(RH_RF95_REG_12_IRQ_FLAGS, 0x40);

    int8_t irqFlags = readRegister(RH_RF95_REG_12_IRQ_FLAGS);

    if ((irqFlags & RH_RF95_PAYLOAD_CRC_ERROR_MASK) == RH_RF95_PAYLOAD_CRC_ERROR_MASK) {
        writeRegister(RH_RF95_REG_12_IRQ_FLAGS, RH_RF95_PAYLOAD_CRC_ERROR_MASK);
        return false;
    } else {
        int8_t currentAddr = readRegister(RH_RF95_REG_10_FIFO_RX_CURRENT_ADDR);
        int8_t receivedCount = readRegister(RH_RF95_REG_13_RX_NB_BYTES);
        receivedBytes = receivedCount;

        writeRegister(RH_RF95_REG_0D_FIFO_ADDR_PTR, currentAddr);

        for(int i = 0; i < receivedCount; i++) {
            payload[i] = (char)readRegister(RH_RF95_REG_00_FIFO);
        }
    }
    return true;
}

void setupRadio()
{
    digitalWrite(pinReset, HIGH);
    delay(100);
    digitalWrite(pinReset, LOW);
    delay(100);

    int8_t version = readRegister(RH_RF95_REG_42_VERSION);

    if (version == 0x22) {
        // sx1272
        printf("SX1272 detected, starting.\n");
        sx1272 = true;
    } else {
        // sx1276?
        digitalWrite(pinReset, LOW);
        delay(100);
        digitalWrite(pinReset, HIGH);
        delay(100);
        version = readRegister(RH_RF95_REG_42_VERSION);
        if (version == 0x12) {
            // sx1276
            printf("SX1276 detected, starting.\n");
            sx1272 = false;
        } else {
            printf("Unrecognized transceiver\n");
            printf("Version: 0x%x\n",version);
            exit(1);
        }
    }

    writeRegister(RH_RF95_REG_01_OP_MODE, SX72_MODE_SLEEP);

    uint32_t frf = (915.0 * 1000000.0) / RH_RF95_FSTEP;
    writeRegister(RH_RF95_REG_06_FRF_MSB, (uint8_t)((frf >> 16) & 0xff));
    writeRegister(RH_RF95_REG_07_FRF_MID, (uint8_t)((frf >> 8) & 0xff));
    writeRegister(RH_RF95_REG_08_FRF_LSB, (uint8_t)((frf) & 0xff));

    if (sx1272) {
        if (sf == SF11 || sf == SF12) {
            writeRegister(RH_RF95_REG_1D_MODEM_CONFIG1, 0x0B);
        } else {
            writeRegister(RH_RF95_REG_1D_MODEM_CONFIG1, 0x0A);
        }
        writeRegister(RH_RF95_REG_1E_MODEM_CONFIG2, (sf << 4)/* | 0x04*/);
    }

    if (sf == SF10 || sf == SF11 || sf == SF12) {
        writeRegister(RH_RF95_REG_1F_SYMB_TIMEOUT_LSB, 0x05);
    } else {
        writeRegister(RH_RF95_REG_1F_SYMB_TIMEOUT_LSB, 0x08);
    }
    writeRegister(RH_RF95_REG_23_MAX_PAYLOAD_LENGTH, 0x80);
    writeRegister(RH_RF95_REG_22_PAYLOAD_LENGTH, PAYLOAD_LENGTH);
    writeRegister(RH_RF95_REG_24_HOP_PERIOD, 0xFF);
    writeRegister(RH_RF95_REG_0D_FIFO_ADDR_PTR, readRegister(RH_RF95_REG_0F_FIFO_RX_BASE_ADDR));

    // Set Continous Receive Mode
    writeRegister(RH_RF95_REG_0C_LNA, RH_RF95_LNA_LOW_GAIN);
    writeRegister(RH_RF95_REG_01_OP_MODE, SX72_MODE_RX_CONTINUOS);

    writeRegister(RH_RF95_REG_09_PA_CONFIG, 0x8f);
    writeRegister(RH_RF95_REG_1F_SYMB_TIMEOUT_LSB, 0x64);
    writeRegister(RH_RF95_REG_22_PAYLOAD_LENGTH, 0x01);
    writeRegister(RH_RF95_REG_23_MAX_PAYLOAD_LENGTH, 0xff);
    writeRegister(RH_RF95_REG_24_HOP_PERIOD, 0x0);
    writeRegister(RH_RF95_REG_26_MODEM_CONFIG3, 0x0);

    printRegisters();
}

void receivePacket() {
    long int snr;
    int32_t rssiCorrection;
    if (sx1272) {
        rssiCorrection = 139;
    } else {
        rssiCorrection = 157;
    }

    if (digitalRead(pinDio0) == 1) {
        if (receivePkt(message)) {
            int8_t value = readRegister(RH_RF95_REG_19_PKT_SNR_VALUE);
            if (value & 0x80) // check for negative
            {
                value = ((~value + 1) & 0xFF) >> 2;
                snr = -value; // Invert and divide by 4
            }
            else
            {
                snr = (value & 0xFF) >> 2; // Divide by 4
            }

            printf("Packet RSSI: %d, ", readRegister(0x1A) - rssiCorrection);
            printf("RSSI: %d, ", readRegister(0x1B) - rssiCorrection);
            printf("SNR: %li, ", snr);
            printf("Length: %i", (int32_t)receivedBytes);
            printf("\n");

            // int8_t headerTo = message[0];
            // int8_t headerFrom = message[1];
            // int8_t headerId = message[2];
            // int8_t headerFlags = message[3];

            message[receivedBytes] = 0;
            printf("%s\n", message + 4);

            fflush(stdout);

        }
    }
}

uint32_t counter = 0;

void handleIsr() {
    counter++;

    int8_t irqFlags = readRegister(RH_RF95_REG_12_IRQ_FLAGS);
}

int32_t main () {
    struct timeval nowtime;
    uint32_t lastSeconds;

    wiringPiSetup();

    pinMode(pinSelect, OUTPUT);
    pinMode(pinDio0, INPUT);
    pinMode(pinReset, OUTPUT);

    wiringPiSPISetup(SPI_CHANNEL, 500000);

    setupRadio();

    wiringPiISR(pinDio0, INT_EDGE_RISING, handleIsr);

    while (true) {
        receivePacket();

        gettimeofday(&nowtime, NULL);
        uint32_t nowSeconds = (uint32_t)(nowtime.tv_sec);
        if (nowSeconds - lastSeconds >= 5) {
            printf("IRQs %d\n", counter);
            lastSeconds = nowSeconds;
        }
        delay(1);
    }

    return (0);

}

