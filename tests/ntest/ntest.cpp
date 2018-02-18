#include "ntest.h"

#include <vector>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <string.h> // strcmp

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

TestBase::Result TestBase :: run(std::ostream& log, int depth) {
    struct Guard {
        TestBase* const b;
        const uint64_t before;
        Guard(TestBase* _b) : b(_b), before(GetTimeMicroseconds()) {}
        ~Guard() { b->time_mcs = GetTimeMicroseconds() - before; }
    } guard(this);
    result = INIT;
    runInternal(log, depth);
    return result;
}

const char* TestBase :: getName() const {
    return name;
}

uint64_t TestBase :: getDurationMicroseconds() const {
    return time_mcs;
}

void TestBase :: PushRunner(TestBase* test) {
    GetRunners().push_back(test);
}

bool TestBase :: hasntFailed() const {
    return result != FAIL;
}

const char* TestBase :: ResultToStr(Result result) {
    switch (result) {
    case INIT: return "?";
    case SUCCESS: return "PASS";
    case SKIP: return "SKIPPED";
    case FAIL: return "FAILED";
    case CRASH: return "CRASH";
    };
    return nullptr;
}

int TestBase :: BoundVMinMax(int v, int min, int max) {
    if(min > max) {
        throw std::runtime_error("min > max in BoundVMinMax");
    }
    if(v < min) {
        return min;
    }
    if(v > max) {
        return max;
    }
    return v;
}

size_t TestBase :: RunTests(std::ostream& log_output, int depth) {
    const auto comp = [](const TestBase* left, const TestBase* right) {
        return strcmp(left->getName(), right->getName()) < 0;
    };
    auto& tests = GetRunners();
    std::sort(tests.begin(), tests.end(), comp);
    size_t n_failed = 0;
    size_t n_succeeded = 0;
    size_t n_skipped = 0;
    size_t n_crashed = 0;
    log_output << "ntest" << NTEST_VERSION_STRING << ": Running tests with depth = " << depth << std::endl;
    double total_time = 0;
    for(auto runner : tests) {
        const auto name = runner->getName();
        Result result = INIT;
        try {
            result = runner->run(log_output, depth);
        } catch (const std::exception& e) {
            log_output << "* test " << name << " thrown exception: " << e.what() << std::endl;
            result = CRASH;
        } catch (...) {
            result = CRASH;
        }
        switch (result) {
        case SUCCESS: ++n_succeeded; break;
        case SKIP: ++n_skipped; break;
        case CRASH: ++n_crashed; // fallthru
        default: ++n_failed; break;
        }
        const auto seconds = (double(runner->getDurationMicroseconds()) / 1000000.);
        total_time += seconds;
        log_output << '[' << std::setw(7) << std::left << ResultToStr(result) << "] " << std::fixed << seconds << " : " << name << std::endl;
    }
    log_output << "ntest: Total tests run : " << (n_failed + n_succeeded) << " in " << std::fixed << total_time << " seconds" << std::endl;
    log_output << "ntest: Succeeded : " << n_succeeded << std::endl;
    log_output << "ntest: Skipped : " << n_skipped << std::endl;
    log_output << "ntest: Failed : " << n_failed;
    if(n_crashed) {
        log_output << " (crashed: " << n_crashed << ")";
    }
    log_output << std::endl;
    return n_failed;
}

void TestBase :: skip() {
    result = SKIP;
}

bool TestBase :: approve(bool condition) {
    if(result != FAIL) {
        result = condition ? SUCCESS : FAIL;
        return result == FAIL;
    }
    return false;
}

} // end namespace ntest
