set "compile_g=gcc tests.cpp ../ecl-all-c-included/ECL_all_c_included.c -O3 -std=c++11 -lstdc++ -DECL_BUILD_AS_C -DECL_USE_ASSERT -DECL_TEST_LOCAL -DECL_USE_STAT_COUNTERS -DECL_USE_BITNESS"
set "run_tests_g=a.exe %1"
del a.exe && %compile_g%_32 && %run_tests_g%
