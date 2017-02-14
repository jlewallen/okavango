#!/bin/bash

set -e
set -x

pushd sensors
gradle clean
popd

pushd ngdemo
gradle clean
popd 

pushd collector
gradle clean
popd
