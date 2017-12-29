set "compile_m=cl tests.cpp all_c.c /O2 /EHsc /DECL_BUILD_AS_C /DECL_USE_ASSERT /DECL_TEST_LOCAL /DECL_USE_STAT_COUNTERS /DECL_USE_BITNESS"
set "run_tests_m=tests.exe"
del a.exe && %compile_m%_32 && %run_tests_m%
