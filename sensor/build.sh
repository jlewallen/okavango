#!/bin/bash
#
# TODO:
# Make showPorts cross platform.

set -e
set -x

ARD_HOME="../arduino-1.6.9"
ARD_BIN="${ARD_HOME}/hardware/tools/avr/bin"
PROJECT_DIR=`pwd`
BUILD_DIR="${PROJECT_DIR}/build"
PORT="COM7"
SOURCES=*.ino

echo $SOURCES

# No editing should be required below here.

function clean() {
    rm -rf ${BUILD_DIR}
    mkdir -p ${BUILD_DIR}
}

function preferences() {
    ${ARD_HOME}/arduino-builder -dump-prefs -logger=machine -hardware "${ARD_HOME}/hardware" -tools "${ARD_HOME}/tools-builder" -tools "${ARD_HOME}/hardware/tools/avr" -built-in-libraries "${ARD_HOME}/libraries" -libraries "../libraries" -fqbn=arduino:avr:uno -ide-version=10609 -build-path "${BUILD_DIR}" -warnings=none -prefs=build.warn_data_percentage=75 -verbose ${SOURCES}
}

function build() {
    ${ARD_HOME}/arduino-builder -compile -logger=machine -hardware "${ARD_HOME}/hardware" -tools "${ARD_HOME}/tools-builder" -tools "${ARD_HOME}/hardware\tools\avr" -built-in-libraries "${ARD_HOME}/libraries" -libraries "../libraries" -fqbn=arduino:avr:uno -ide-version=10609 -build-path "${BUILD_DIR}" -warnings=none -prefs=build.warn_data_percentage=75 -verbose ${SOURCES}
}

function upload() {
    # Avrdude dislikes cygwin style paths.
    BUILD_DIR_WINDOWS=`echo ${BUILD_DIR} | sed -r "s|/([a-zA-Z])/|\1:/|"`
    ${ARD_HOME}/hardware/tools/avr/bin/avrdude -C${ARD_HOME}/hardware/tools/avr/etc/avrdude.conf -v -patmega328p -carduino -P${PORT} -b115200 -D -Uflash:w:${BUILD_DIR_WINDOWS}/blink.ino.hex:i 
}

function showPorts() {
    wmic path Win32_SerialPort get deviceid, description
}

if [ .$1 = . -o .$1 = .-h ]; then
    echo "usage: `basename $0` [-p] [-c] [-u] [-b] sketch-file.ino <other sources>"
    echo "        -p show ports"
    echo "        -c clean"
    echo "        -u build and upload"
    echo "        -b build"
    exit 1
fi

while [ .${1:0:1} = .- ]; do
    if [ .$1 = .-p ]; then showPorts
    elif [ .$1 = .-b ]; then clean && build
    elif [ .$1 = .-u ]; then clean && build && upload
    elif [ .$1 = .-c ]; then clean
    else
        echo "Unknown option $1"
        exit 1
    fi
    shift
done
