#!/bin/sh
set -e
clang sample.cpp -std=c++11 -lstdc++ -Werror -Wall -pedantic -O3 -o ecl
