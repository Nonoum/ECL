// optional define
#define NTEST_NAMESPACE_NAME ntest_for_ecl

#include "tests_base_inline.cpp"

int main(int argc, char* argv[]) {
    int depth = 0;
    if(argc == 2) {
        depth = atoi(argv[1]);
    }
    return NTEST_NAMESPACE_NAME::TestBase::RunTests(std::cout, depth);
}
