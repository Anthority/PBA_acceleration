#!/usr/bin/bash
dir="bigbenchmark"
case="
des_perf
"
# ac97_ctrl
# aes_core
# vga_lcd

# c3540
# c6288
# c7552
slews="0.1 0.001 0.0001"
pathnum=1000000
for file in $case
    do
    echo $file
    for slew in $slews
        do
            ./cmp_run.sh $dir $file $pathnum $slew 0
        done
    done