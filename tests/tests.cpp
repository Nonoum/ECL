#include "ntest/ntest.cpp"
#include "../ECL_common.c"
#include "../ECL_ZeroEater.c"
#include "../ECL_ZeroDevourer.c"
#include "tests_common.cpp.inl"
#include "tests_datasets.cpp.inl"
#include "tests_random_data.cpp.inl"
#include "tests_perf.cpp.inl"

NTEST(test_string_constants) {
    // test some weird stuff used in other tests for comfortability
    approve(char(3) == "\x3\x5"[0]);
    approve(char(5) == "\x3\x5"[1]);
    approve(char(5) == "\x0\x5"[1]);
    approve(char(0x70) == "\x70\x75"[0]);
    approve(char(0x75) == "\x70\x75"[1]);
}

int main() {
    return ntest::TestBase::RunTests(std::cout);
}
