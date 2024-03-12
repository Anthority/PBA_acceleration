#!/usr/bin/bash
cd $1
cd cmake-build-debug
cmake ..
make -j 8
cd ..
./$1 > test.log
cd ..