#!/usr/bin/bash

# Remember the first argument is the benchmark path

FILE=$1/$2/main.cpp
if [ -f "$FILE" ]; then
    echo "$FILE exists."
else
    echo "$FILE does not exist."
    cp Doc/main_template.cpp $1/$2/main.cpp
fi

cd build
cmake .. -DBENCHMARK:STRING=$1 -DCASE:STRING=$2
make -j 8
cd ..
cd $1/$2/
./$2.exe $2 $3 $4 > test.log
cd ..