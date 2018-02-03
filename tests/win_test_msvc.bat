if "%1"=="deep" (
    set "extra=/DECL_TEST_DEEP"
) else (
    set "extra="
)
set "compile=cl tests.cpp all_c.c /O2 /EHsc %extra% /DECL_BUILD_AS_C /DECL_USE_ASSERT /DECL_USE_BITNESS"
set "run_tests=tests.exe"
del tests.exe && %compile%_16 && %run_tests% && %compile%_32 && %run_tests% && %compile%_64 && %run_tests% && pause
