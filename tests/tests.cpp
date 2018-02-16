#include "ntest/ntest.cpp"
#include "../ECL_JH_States.h"
#include "../ECL_ZeroEater.h"
#include "../ECL_ZeroDevourer.h"
#include "../ECL_NanoLZ.h"

#ifndef ECL_BUILD_AS_C
#include "../ECL_common.c"
#include "../ECL_ZeroEater.c"
#include "../ECL_ZeroDevourer.c"
#include "../ECL_NanoLZ.c"
#endif

#include <fstream>

// auxiliary macro and methods for testing
#define ECL_TEST_ASSERT(expr) approve((bool)(expr))
#define ECL_TEST_COMPARE(val1, val2)                               \
    {                                                              \
        if((val1) != (val2)) {                                     \
            if(hasntFailed()) {                                    \
                log << "VAL1: " << std::hex << (val1) << " != VAL2: " << (val2) << " "; \
            }                                                      \
            approve(false);                                        \
        } else {                                                   \
            approve(true);                                         \
        }                                                          \
    }

#ifdef ECL_TEST_DEEP
#define ECL_TEST_IS_DEEP 1
#else
#define ECL_TEST_IS_DEEP 0
#endif

#define ECL_TEST_MAGIC_RESIZE(vector_name, capacity) \
    vector_name.resize(capacity + 1); \
    vector_name[capacity] = 0x39;

#define ECL_TEST_MAGIC_VALIDATE(vector_name) \
    ECL_TEST_ASSERT(vector_name[vector_name.size() - 1] == 0x39)



void ECL_TEST_LogRawData(std::ostream& log, const std::vector<uint8_t>& v) {
    const size_t max_size = 50;
    auto sz = std::min(max_size, v.size());
    log << '{' << v.size() << '}';
    log << '[';
    for(size_t i = 0; i < sz; ++i) {
        if(i) {
            log << ", ";
        }
        log << std::hex << int(v[i]);
    }
    if(v.size() > sz) {
        log << ", ...";
    }
    log << ']' << std::endl;
}

#ifndef ECL_TEST_LOCAL
#include "tests_common_inline.hpp"
#include "tests_datasets_inline.hpp"
#include "tests_random_data_inline.hpp"
#include "tests_perf_inline.hpp"
#include "tests_errors_inline.hpp"
#endif

NTEST(test_string_constants) {
    NTEST_SUPPRESS_UNUSED;
    // test some weird stuff used in other tests for comfortability
    approve(char(3) == "\x3\x5"[0]);
    approve(char(5) == "\x3\x5"[1]);
    approve(char(5) == "\x0\x5"[1]);
    approve(char(0x70) == "\x70\x75"[0]);
    approve(char(0x75) == "\x70\x75"[1]);
}

NTEST(test_ECL_LogSize) {
    NTEST_SUPPRESS_UNUSED;
    ECL_TEST_COMPARE(ECL_LogRoundUp(0), 1);
    ECL_TEST_COMPARE(ECL_LogRoundUp(1), 1);
    ECL_TEST_COMPARE(ECL_LogRoundUp(2), 1);
    ECL_TEST_COMPARE(ECL_LogRoundUp(3), 2);
    ECL_TEST_COMPARE(ECL_LogRoundUp(4), 2);
    ECL_TEST_COMPARE(ECL_LogRoundUp(5), 3);
    ECL_TEST_COMPARE(ECL_LogRoundUp(8), 3);
    ECL_TEST_COMPARE(ECL_LogRoundUp(9), 4);
    ECL_TEST_COMPARE(ECL_LogRoundUp(16), 4);
    ECL_TEST_COMPARE(ECL_LogRoundUp(17), 5);
    ECL_TEST_COMPARE(ECL_LogRoundUp(32), 5);
    ECL_TEST_COMPARE(ECL_LogRoundUp(33), 6);
    ECL_TEST_COMPARE(ECL_LogRoundUp(64), 6);
    ECL_TEST_COMPARE(ECL_LogRoundUp(65), 7);
    ECL_TEST_COMPARE(ECL_LogRoundUp(128), 7);
    ECL_TEST_COMPARE(ECL_LogRoundUp(129), 8);
}

#ifdef ECL_TEST_LOCAL

#include "stat_dir_inline.hpp"

NTEST(test_stat_dir) {
    log << std::endl;
    approve(ECL_Stat_Dir_Files_NanoLZ("C:/test-data/2", ECL_NANOLZ_SCHEME1, "-- ", log));
}

#endif

int main() {
    return ntest::TestBase::RunTests(std::cout);
}
