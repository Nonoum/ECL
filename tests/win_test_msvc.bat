set "compile=cl tests.cpp ../ecl-all-c-included/ECL_all_c_included.c /O2 /EHsc /DECL_BUILD_AS_C /DECL_USE_ASSERT /DECL_USE_BITNESS"
set "run_tests=tests.exe %1"
del tests.exe && %compile%_16 && %run_tests% && %compile%_32 && %run_tests% && %compile%_64 && %run_tests% && pause
