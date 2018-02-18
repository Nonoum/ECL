set "compile_m=cl tests.cpp ../ecl-all-c-included/ECL_all_c_included.c /O2 /EHsc /DECL_BUILD_AS_C /DECL_USE_ASSERT /DECL_TEST_LOCAL /DECL_USE_STAT_COUNTERS /DECL_USE_BITNESS"
set "run_tests_m=tests.exe %1"
del a.exe && %compile_m%_32 && %run_tests_m%
