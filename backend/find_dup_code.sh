#!/bin/sh

code=$1

if [ -n "$code" ]; then
    grep -RE "#define\s+\w+\s+$code" .
else
    echo "usage:
    ./find_dup_code.sh 1510"
fi

