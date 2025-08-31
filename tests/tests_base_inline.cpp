#include "ntest/ntest.cpp"
#include "../ECL_JH_States.h"
#include "../ECL_ZeroEater.h"
#include "../ECL_ZeroDevourer.h"
#include "../ECL_NanoLZ.h"
#include "../ECL_Huff8.h"
#include "../ECL_utils.h"

#ifndef ECL_BUILD_AS_C
#include "../ECL_common.c"
#include "../ECL_ZeroEater.c"
#include "../ECL_ZeroDevourer.c"
#include "../ECL_NanoLZ.c"
#include "../ECL_Huff8.c"
#endif

#include <fstream>

// auxiliary macros and methods for testing
#define ECL_TEST_ASSERT       NTEST_ASSERT
#define ECL_TEST_ASSERT_EX    NTEST_ASSERT_EX

#define ECL_TEST_COMPARE      NTEST_COMPARE
#define ECL_TEST_COMPARE_EX   NTEST_COMPARE_EX


#define ECL_TEST_MAGIC_RESIZE(vector_name, capacity) \
    vector_name.assign(capacity + 1, 0); \
    vector_name[capacity] = 0x39; NTEST_NAMESPACE_NAME::ntest_noop()

#define ECL_TEST_MAGIC_VALIDATE(vector_name) \
    ECL_TEST_ASSERT(vector_name[vector_name.size() - 1] == 0x39)

#define ECL_TEST_MAGIC_LOG_TOUCHED_SIZE(vector_name, str_description) \
    log << str_description << [&vector_name](){ return std::find_if(vector_name.rbegin() + 1, vector_name.rend(), [](auto& v){ return v != 0; }).base() - vector_name.begin(); }() << "\n";


template <typename T>
static void ECL_TEST_LogVectorData(std::ostream& log, const std::vector<T>& v, size_t max_size = size_t(-1)) {
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
    log << ']';
}

#include "tests_common_inline.hpp"
#include "tests_datasets_inline.hpp"
#include "tests_random_data_inline.hpp"
#include "tests_perf_inline.hpp"
#include "tests_errors_inline.hpp"
#include "tests_huff8_specific_inline.hpp"

NTEST(test_version) {
    NTEST_SUPPRESS_UNUSED;
    log << "  ECL Size Bitness: " << ECL_GetSizeBitness() << std::endl;
    log << "  ECL Version: " << ECL_GetVersionNumber() << std::endl;
    log << "  ECL Version String: " << ECL_GetVersionString() << std::endl;
    log << "  ECL Version Branch: " << ECL_GetVersionBranch() << std::endl;
    approve(true);
}
