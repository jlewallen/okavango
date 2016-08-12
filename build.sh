#!/bin/bash

set -e
set -x

BUILD=`pwd`/building/build.sh

pushd tests
for a in *; do
    pushd $a
    $BUILD -m -b
    $BUILD -a -b
    popd
done
popd

pushd sensors
$BUILD -m -b
popd 

pushd collector
$BUILD -m -b
popd

pushd weather-shield
$BUILD -a -b
popd

pushd fona-driver
$BUILD -f -b
popd

