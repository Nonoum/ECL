#include "ntest/ntest.cpp"
#include "../ECL_JH_States.h"
#include "../ECL_ZeroEater.h"
#include "../ECL_ZeroDevourer.h"
#include "../ECL_NanoLZ.h"
#include "../ECL_utils.h"

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

#define ECL_TEST_MAGIC_RESIZE(vector_name, capacity) \
    vector_name.resize(capacity + 1); \
    vector_name[capacity] = 0x39; NTEST_NAMESPACE_NAME::ntest_noop()

#define ECL_TEST_MAGIC_VALIDATE(vector_name) \
    ECL_TEST_ASSERT(vector_name[vector_name.size() - 1] == 0x39)



static void ECL_TEST_LogRawData(std::ostream& log, const std::vector<uint8_t>& v) {
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

#include "tests_common_inline.hpp"
#include "tests_datasets_inline.hpp"
#include "tests_random_data_inline.hpp"
#include "tests_perf_inline.hpp"
#include "tests_errors_inline.hpp"

NTEST(test_version) {
    NTEST_SUPPRESS_UNUSED;
    log << "  ECL Size Bitness: " << ECL_GetSizeBitness() << std::endl;
    log << "  ECL Version: " << ECL_GetVersionNumber() << std::endl;
    log << "  ECL Version String: " << ECL_GetVersionString() << std::endl;
    log << "  ECL Version Branch: " << ECL_GetVersionBranch() << std::endl;
    approve(true);
}
