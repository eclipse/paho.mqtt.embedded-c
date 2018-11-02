#!/bin/bash

set -e

rm -rf build.paho
mkdir build.paho
cd build.paho
echo "travis build dir $TRAVIS_BUILD_DIR pwd $PWD"
cmake ..
make
python3 ../test/mqttsas.py localhost 1883 1885 &
ctest -VV --timeout 600
kill %1
#killall mosquitto
