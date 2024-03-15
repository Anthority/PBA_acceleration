#!/usr/bin/bash

# Remember the first argument is the benchmark path

FILE=$1/$2/main.cpp
if [ -f "$FILE" ]; then
    echo "$FILE exists."
    cp Doc/main_template.cpp $1/$2/main.cpp
else
    echo "$FILE does not exist."
    cp Doc/main_template.cpp $1/$2/main.cpp
fi

if [ "$6" = "debug" ]; then
    cd build_debug
    rm -f CMakeCache.txt
    cmake .. -DBENCHMARK:STRING=$1 -DCASE:STRING=$2  -DCMAKE_BUILD_TYPE=Debug
else
    cd build_release
    rm -f CMakeCache.txt
    cmake .. -DBENCHMARK:STRING=$1 -DCASE:STRING=$2
fi

make -j 64
cd ..
cd $1/$2/
./$2.exe $2 $3 $4 $5 > test.log
cd ..