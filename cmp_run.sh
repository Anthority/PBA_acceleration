#!/usr/bin/bash
cd $1
cd build
cmake ..
make -j64
cd ..
./$1 > test.log
cd ..