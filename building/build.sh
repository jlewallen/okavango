#!/bin/bash
#
# TODO:
# Make showPorts cross platform.

set -e
set -x

ARD_HOME="../arduino-1.6.9"
ARD_BIN="${ARD_HOME}/hardware/tools/avr/bin"
PROJECT_DIR=`pwd`
ROOT_DIR="${PROJECT_DIR}/../"
BUILDING_DIR="${PROJECT_DIR}/../building"
BUILD_DIR="${PROJECT_DIR}/build"
SOURCES=*.ino
BOSSAC=../arduino/packages/arduino/tools/bossac/1.6.1-arduino/bossac.exe

function use_uno() {
    BOARD="arduino:avr:uno"
    MCU="atmega328p"
    PROGRAMMER="avrdude"
    AVR_DUDE_PROGRAMMER="arduino"
    BAUD="115200"
    AB_EXTRA=""
    PORT_NAME="Arduino"
}

function use_feather32u4() {
    BOARD="adafruit:avr:feather32u4"
    MCU="atmega32u4"
    PROGRAMMER="avrdude"
    AVR_DUDE_PROGRAMMER="avr109"
    BAUD="57600"
    AB_EXTRA="-vid-pid=0X239A_0X800C"
    PORT_NAME="Adafruit"
}

function use_featherm0() {
    BOARD="adafruit:samd:adafruit_feather_m0"
    MCU=""
    PROGRAMMER="bossac"
    BAUD="57600"
    AB_EXTRA=""
    PORT_NAME="Adafruit"
}

# No editing should be required below here.

function get_port() {
    PORT=`node ${BUILDING_DIR}/get-port.js monitor $BOARD $PORT_NAME $PORT`
}

function clean() {
    rm -rf ${BUILD_DIR}
    mkdir -p ${BUILD_DIR}
}

function preferences() {
    ${ARD_HOME}/arduino-builder -dump-prefs -logger=machine \
     -hardware "${ARD_HOME}/hardware" -hardware "${PROJECT_DIR}/../arduino/packages" \
     -tools "${ARD_HOME}/tools-builder" -tools "${ARD_HOME}/hardware/tools/avr" -tools "${PROJECT_DIR}/../arduino/packages" \
     -built-in-libraries "${ARD_HOME}/libraries" -libraries "../libraries" -fqbn=${BOARD} ${AB_EXTRA} -ide-version=10609  \
     -build-path ${BUILD_DIR} -warnings=none -prefs=build.warn_data_percentage=75 -verbose ${SOURCES}
}

function build() {
    ${ARD_HOME}/arduino-builder -compile -logger=machine \
     -hardware "${ARD_HOME}/hardware" -hardware "${PROJECT_DIR}/../arduino/packages" \
     -tools "${ARD_HOME}/tools-builder" -tools "${ARD_HOME}/hardware/tools/avr" -tools "${PROJECT_DIR}/../arduino/packages" \
     -built-in-libraries "${ARD_HOME}/libraries" -libraries "../libraries" -fqbn=${BOARD} ${AB_EXTRA} -ide-version=10609 \
     -build-path ${BUILD_DIR} -warnings=none -prefs=build.warn_data_percentage=75 -verbose ${SOURCES}
}

function upload() {
    UPLOAD_PORT=`node ${BUILDING_DIR}/get-port.js upload $BOARD $PORT_NAME $PORT`

    if [ "$PROGRAMMER" == "avrdude" ]; then
        # So, avrdude dislikes cygwin style paths.
        BUILD_DIR_WINDOWS=`echo ${BUILD_DIR} | sed -r "s|/([a-zA-Z])/|\1:/|"`
        BINARY=`echo ${BUILD_DIR_WINDOWS}/*.ino.hex`
        ${ARD_HOME}/hardware/tools/avr/bin/avrdude -C${ARD_HOME}/hardware/tools/avr/etc/avrdude.conf -v -p${MCU} -c${AVR_DUDE_PROGRAMMER} -P${UPLOAD_PORT} -b${BAUD} -D -Uflash:w:${BINARY}:i
    fi

    if [ "$PROGRAMMER" == "bossac" ]; then
        BINARY=`echo ${BUILD_DIR}/*bin`
        ${BOSSAC}  -i -d --port=${UPLOAD_PORT} -U true -i -e -w -v ${BINARY} -R 
    fi
}

function show_ports() {
    wmic path Win32_SerialPort get deviceid, description
}

function open_serial() {
    rm -f log.txt
    MONITOR_PORT=`node ${BUILDING_DIR}/get-port.js watch $BOARD $PORT_NAME $PORT`
    ../setup/putty.exe -serial $PORT -sercfg 115200 -sessionlog log.txt
}

function size() {
    BINARY=`echo ${BUILD_DIR}/*.ino.hex`
    ${ARD_HOME}/hardware/tools/avr/bin/avr-size ${BINARY}
    ${ARD_HOME}/hardware/tools/avr/bin/avr-size -C --mcu=atmega328p ${BINARY}
}

if [ .$1 = . -o .$1 = .-h ]; then
    echo "usage: `basename $0` [-fa] [-p] [-c] [-u] [-b] sketch-file.ino <other sources>"
    echo "        -f feather"
    echo "        -a arduino uno"
    echo "        -p show ports"
    echo "        -c clean"
    echo "        -u build and upload"
    echo "        -b build"
    exit 1
fi

while [ .${1:0:1} = .- ]; do
    if [ .$1 = .-p ]; then show_ports
    elif [ .$1 = .-f ]; then use_feather32u4
    elif [ .$1 = .-a ]; then use_uno
    elif [ .$1 = .-m ]; then use_featherm0
    elif [ .$1 = .-b ]; then clean && build
    elif [ .$1 = .-u ]; then get_port && upload
    elif [ .$1 = .-c ]; then clean
    elif [ .$1 = .-s ]; then get_port && open_serial
    elif [ .$1 = .-z ]; then size
    else
        echo "Unknown option $1"
        exit 1
    fi
    shift
done
