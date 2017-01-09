#!/bin/bash

set -e
set -x

pushd sensors
gradle clean build
popd

pushd ngdemo
gradle clean build
popd 

pushd collector
gradle clean build
popd
