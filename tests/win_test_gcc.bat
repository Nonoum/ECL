set "compile=gcc tests.cpp all_c.c -m32 -Wall -O3 -std=c++11 -lstdc++ -DECL_BUILD_AS_C -DECL_USE_ASSERT -DECL_USE_BITNESS"
set "run_tests=a.exe"
del a.exe && %compile%_16 && %run_tests% && %compile%_32 && %run_tests% && %compile%_64 && %run_tests% && pause
