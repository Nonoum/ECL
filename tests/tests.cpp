#include "ntest/ntest.cpp"
#include "../ECL_common.c"
#include "../ECL_ZeroEater.c"
#include "../ECL_ZeroDevourer.c"
#include "tests_common.cpp.inl"
#include "tests_datasets.cpp.inl"

int main() {
    return ntest::TestBase::RunTests(std::cout);
}
