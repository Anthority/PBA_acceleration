#!/usr/bin/bash

# Remember the first argument is the benchmark path
# $1: benchmark path
# $2: case name
# $3: path num
# $4: acceptable slew（可接受的slew误差范围）
# $5: debug or release

# example:
# ./cmp_run.sh benchmark optimizer 1000 0.1 debug
# ./cmp_run.sh benchmark vga_lcd 1000000 0.1

###############################################################
# 如果main.cpp不存在，则拷贝doc/main_template.cpp到$1/$2/main.cpp
FILE=$1/$2/main.cpp
diff $FILE doc/main_template.cpp > /dev/null

if [ $? == 0 ]; then
    echo "$FILE exists."
else
    echo "$FILE does not exist."
    cp doc/main_template.cpp $1/$2/main.cpp
fi

###############################################################
# 决定是debug模式还是release模式
if [ "$5" = "debug" ]; then
    cd build_debug
    rm -f CMakeCache.txt
    cmake .. -DBENCHMARK:STRING=$1 -DCASE:STRING=$2  -DCMAKE_BUILD_TYPE=Debug
else
    cd build_release
    rm -f CMakeCache.txt
    cmake .. -DBENCHMARK:STRING=$1 -DCASE:STRING=$2
fi

###############################################################
# 运行并将log输出到log文件夹
make -j 64
cd ..
cd $1/$2/
./$2.exe $2 $3 $4 > ../../log/$2.log
cd ..