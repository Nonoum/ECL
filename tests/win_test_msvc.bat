set "compile=cl tests.cpp all_c.c /O2 /EHsc /DECL_BUILD_AS_C /DECL_USE_ASSERT /DECL_USE_BITNESS"
set "run_tests=tests.exe"
del tests.exe && %compile%_16 && %run_tests% && %compile%_32 && %run_tests% && %compile%_64 && %run_tests%
