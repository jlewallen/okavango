#!/bin/bash

set -e
set -x

pushd sensors
gradle build
popd

pushd ngdemo
gradle build
popd 

pushd collector
gradle build
popd
