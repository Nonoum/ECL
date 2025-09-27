#!/bin/sh
set -e
if [ -z "$1" ]
  then
    echo "specify sample *.cpp file name to compile"
  else
    clang $1 -std=c++17 -lstdc++ -Werror -Wall -pedantic -O3 -o ecl
fi
