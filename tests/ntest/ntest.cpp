#include "ntest.h"

#include <algorithm>
#include <string.h>

static uint64_t RdtscWrapper() {
    union {
        uint64_t i64;
        struct {
            uint32_t low;
            uint32_t high;
        } i32;
    } value;
#ifdef _MSC_VER // ms visual studio compiler
    uint32_t l, h;
    __asm {
        rdtsc;
        mov l, eax;
        mov h, edx;
       }
    value.i32.low = l;
    value.i32.high = h;
#elif defined __GNUC__ // gcc compiler
    value.i64 = __rdtsc();
#endif
    return value.i64;
}


namespace ntest {

typedef std::vector<TestBase*> RunnersType;

static RunnersType& GetRunners() {
    static RunnersType runners;
    return runners;
}

TestBase :: TestBase(const char* _name)
        : name(_name), result(INIT), tacts(0) {
}

bool TestBase :: run(std::ostream& log) {
    result = INIT;
    uint64_t before = RdtscWrapper();
    runInternal(log);
    uint64_t after = RdtscWrapper();
    tacts = after - before;
    return result == SUCCESS;
}

const char* TestBase :: getName() const {
    return name;
}

uint64_t TestBase :: getTacts() const {
    return tacts;
}

void TestBase :: PushRunner(TestBase* test) {
    auto comp = [](const TestBase* left, const TestBase* right) {
        return strcmp(left->getName(), right->getName()) < 0;
    };
    auto& tests = GetRunners();
    tests.insert(std::upper_bound(tests.begin(), tests.end(), test, comp), test);
}

bool TestBase :: hasntFailed() const {
    return result != FAIL;
}

size_t TestBase :: RunTests(std::ostream& log_output) {
    size_t n_failed = 0;
    size_t n_succeeded = 0;
    log_output << ":: Running tests...." << std::endl;
    for(auto runner : GetRunners()) {
        log_output << runner->getName() << " : ";
        bool result = false;
        try {
            result = runner->run(log_output);
            result ? ++n_succeeded : ++n_failed;
        } catch (...) {
            ++n_failed;
        }
        log_output << (result ? "success\t\t| ~" : "fail\t\t| ~") << std::fixed << (double(runner->getTacts()) / 3000000000.) << std::endl;
    }
    log_output << ":: Total tests : " << (n_failed + n_succeeded) << std::endl;
    log_output << ":: Succeeded : " << n_succeeded << std::endl;
    log_output << ":: Failed : " << n_failed << std::endl;
    return n_failed;
}

void TestBase :: approve(bool condition) {
    if(result != FAIL) {
        result = condition ? SUCCESS : FAIL;
    }
}

}
