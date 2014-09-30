#!/bin/bash

if [ -z "$1" ]; then
        LOG_FILE=val.log;
else
        LOG_FILE="$1";
fi

rm -vf "$LOG_FILE";

function wrap_valgrind () {
        if [ -z "$1" -a -z "$2" ]; then
                echo "args != 2";
                exit 1;
        fi
        valgrind ./bignum "$1" "$2" 2>&1 | cat >> "$LOG_FILE";
}

wrap_valgrind  1024 5
wrap_valgrind  -1024 5
wrap_valgrind  1024 -5
wrap_valgrind  -1024 -5
wrap_valgrind  1024 50
wrap_valgrind  -1024 -5000
grep 'ERROR SUMMARY' "$LOG_FILE";

