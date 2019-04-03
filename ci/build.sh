#!/bin/sh

source /opt/qt511/bin/qt511-env.sh
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .
