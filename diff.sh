#!/usr/bin/bash
file1=$1
file2=$2
diff $file1 $file2 > /dev/null
if [ $? == 0 ]; then
    echo "Both file are same"
else
    echo "Both file are different"
fi