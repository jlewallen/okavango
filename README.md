## Pin Wiring

First, some handy pinout diagrams:

* [Feather M0](https://cdn-shop.adafruit.com/product-files/2796/2796_pinout_v1_0.pdf)
* [Raspberry Pi](http://pinout.xyz/pinout/spi)
* [Arduino Uno](https://blog.arduino.cc/2012/05/29/handy-arduino-uno-r3-pinout-diagram/)


|  Component Pin  | Feather M0 Pin | Arduino Uno Pin | Raspberry Pi 3 |
|:---------------:|:--------------:|:---------------:|:--------------:|
| Conductivity Rx |       10       |        4        |                |
| Conductivity Tx |       11       |        5        |                |
| SPE A (Rx)      |        0       |        2        |                |
| SPE B (Tx)      |        1       |        3        |                |
| SPE X           |        5       |        6        |                |
| SPE Y           |        6       |        7        |                |
| LoRa En         |                |        9        |                |
| LoRa G0         |       18       |        2        |        7       |
| LoRa SCK        |       24       |        13       |       23       |
| LoRa MISO       |       22       |        12       |       21       |
| LoRa MOSI       |       23       |        11       |       19       |
| LoRa CS         |       19       |        10       |       22       |
| LoRa RST        |       17       |                 |       11       |

## Caveats:

### Feather M0

1. Use Vusb/Vbat to power the sensors as the 3.3v pin can't provide enough current.

## Raspberry Pi Setup

Here is the general idea: 

    sudo apt-get update
    sudo apt-get install vim tmux git node python-pip 
    sudo raspi-config # Enable SPI
    sudo raspi-config # Update
    git clone git://git.drogon.net/wiringPi
    cd wiringPi
    ./build
    git clone https://github.com/jlewallen/okavango.git
    cd okavango
    make
    
