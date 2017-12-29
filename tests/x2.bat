set "compile_g=gcc tests.cpp all_c.c -O3 -std=c++11 -lstdc++ -DECL_BUILD_AS_C -DECL_USE_ASSERT -DECL_TEST_LOCAL -DECL_USE_STAT_COUNTERS -DECL_USE_BITNESS"
set "run_tests_g=a.exe"
del a.exe && %compile_g%_32 && %run_tests_g%
