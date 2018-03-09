#!/bin/sh
set -e
gcc sample.cpp -std=c++11 -lstdc++ -Werror -Wall -pedantic -O3 -o ecl
