set "compile=gcc tests.cpp ../ecl-all-c-included/ECL_all_c_included.c -m32 -Wall -Wextra -pedantic -O3 -std=c++11 -lstdc++ -o a.exe -DECL_BUILD_AS_C -DECL_USE_ASSERT -DECL_USE_BITNESS"
set "run_tests=a.exe %1"
del a.exe && %compile%_16 && %run_tests% && %compile%_32 && %run_tests% && %compile%_64 && %run_tests% && pause
