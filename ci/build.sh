#!/bin/sh

source /opt/qt511/bin/qt511-env.sh
if [ ! -d build ]; then
    mkdir build
fi
cd build
# build
cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build . || exit 1
# if succeeds, run unit tests
./cliTest/cliTest && ./GuiUnitTest/GuiUnitTest && ./salaTest/salaTest && ./genlibTest/genlibTest || exit 1
# if that succeeds, run regression tests
cd ../RegressionTest/test && python3.5 test_main.py && cd .. && python3.5 RegressionTestRunner.py && python3.5 RegressionTestRunner.py regressionconfig_agents.json
