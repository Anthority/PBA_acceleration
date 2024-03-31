#!/usr/bin/bash
dir="benchmark"
case="
c6288
c7552
c7552_slack
c880
s1196
s1494
s27
s27_spef
s344
s349
s386
s400
s510
s526
simple
"
for file in $case
    do
        echo $file
        ./cmp_run.sh $dir $file 1000000 0.01f 10 $2
    done