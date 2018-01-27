#include "ntest.h"

#include <algorithm>
#include <string.h>
#include <chrono>

namespace ntest {

uint64_t GetTimeMicroseconds() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

typedef std::vector<TestBase*> RunnersType;

static RunnersType& GetRunners() {
    static RunnersType runners;
    return runners;
}

TestBase :: TestBase(const char* _name)
        : name(_name), result(INIT), time_mcs(0) {
}

bool TestBase :: run(std::ostream& log) {
    result = INIT;
    uint64_t before = GetTimeMicroseconds();
    runInternal(log);
    uint64_t after = GetTimeMicroseconds();
    time_mcs = after - before;
    return result == SUCCESS;
}

const char* TestBase :: getName() const {
    return name;
}

uint64_t TestBase :: getDurationMicroseconds() const {
    return time_mcs;
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
        log_output << (result ? "success\t\t| ~" : "fail\t\t| ~") << std::fixed << (double(runner->getDurationMicroseconds()) / 1000000.) << std::endl;
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
