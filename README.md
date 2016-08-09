## Pin Wiring

First, some handy pinout diagrams:

* [Feather M0](https://cdn-shop.adafruit.com/product-files/2796/2796_pinout_v1_0.pdf)
* [Raspberry Pi](http://pinout.xyz/pinout/spi)
* [Arduino Uno](https://blog.arduino.cc/2012/05/29/handy-arduino-uno-r3-pinout-diagram/)

|    Component    | Feather 32u4 Fona | Feather M0 Lora | Feather M0 Logger | Arduino Uno | Raspberry Pi 3 |
|:---------------:|:-----------------:|:---------------:|:-----------------:|:-----------:|:--------------:|
| Conductivity Rx |                   | 10              | 10                | 4           |                |
| Conductivity Tx |                   | 11              | 11                | 5           |                |
| SPE A (Rx)      |                   | 0               | 0                 | 2           |                |
| SPE B (Tx)      |                   | 1               | 1                 | 3           |                |
| SPE X           |                   | 5               | 5                 | 6           |                |
| SPE Y           |                   | 6               | 6                 | 7           |                |
| LoRa En         |                   |                 |                   | 9           |                |
| LoRa G0         | 2                 | 3               | 18                | 2           | 7              |
| LoRa SCK        | 15                |                 | 24                | 13          | 23             |
| LoRa MISO       | 14                |                 | 22                | 12          | 21             |
| LoRa MOSI       | 16                |                 | 23                | 11          | 19             |
| LoRa CS         | 23                | 8               | 19                | 10          | 22             |
| LoRa RST        | 21                | 4               | 17                |             | 11             |
| SD CS           | 4                 | 16              | 4                 |             |                |
| WS (Rx)         | 1                 |                 |                   |             |                |
| WS (Tx)         | 0                 |                 |                   |             |                |

## Caveats:

### Feather M0

1. Use Vusb/Vbat to power the sensors as the 3.3v pin can't provide enough current.

## Raspberry Pi Setup

Here is the general idea: 

    sudo apt-get update
    sudo apt-get install vim tmux git node python-pip monit
    sudo raspi-config # Enable SPI
    sudo raspi-config # Update
    git clone git://git.drogon.net/wiringPi
    cd wiringPi
    ./build
    git clone https://github.com/jlewallen/okavango.git
    cd okavango
    make
    
## Cron

    sudo cp pi/etc/okavango.cron /etc/cron.d
    sudo /etc/init.d/crond restart
    
## Monit

    sudo cp pi/etc/okavango.moni /etc/monit/conf.d/okavango
    sudo /etc/init.d/monit restart
