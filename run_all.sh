#!/usr/bin/bash
for file in $1/*
    do
        echo $(basename $file)
        ./cmp_run.sh $1 $(basename $file) 1000000 0.01f $2
    done