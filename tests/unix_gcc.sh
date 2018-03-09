#!/bin/sh
depth=$1

main_src="../ecl-all-c-included/ECL_all_c_included.c"
test_src="tests.cpp"
arch="-m32"
opts="$arch -Wall -Wextra -pedantic -O3 -DECL_BUILD_AS_C -DECL_USE_ASSERT"
cpp_opts="-std=c++11"
linker_opts="$arch -lstdc++"
cc="gcc"
cxx="g++"
link="gcc"

# param 1 = bitness
get_opts() {
    echo "$opts -DECL_USE_BITNESS_$1"
}

compile_run() {
    set +e
    rm *.o
    set -e
    $cc $main_src $(get_opts $1) -c
    $cxx $test_src $(get_opts $1) $cpp_opts -c
    $link $linker_opts *.o -o a.out
    echo ./a.out $depth
}

set -x
$(compile_run 16)
$(compile_run 32)
$(compile_run 64)
