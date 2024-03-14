#!/usr/bin/bash
# cd $1
for file in $1/*
    do
        echo $file
        echo $(basename $file)
        rm -f $1/$(basename $file).exe
        rm -rf $file/report
    done
