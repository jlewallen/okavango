#!/bin/bash

set -e
set -x

BUILD=`pwd`/building/build.sh

pushd blink
$BUILD -a -b
$BUILD -m -b
popd

pushd extra-serials
$BUILD -m -b
popd

pushd non-blocking
$BUILD -a -b
$BUILD -m -b
popd

pushd sensor
$BUILD -a -b
popd
