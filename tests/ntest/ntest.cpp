#include "ntest.h"

#include <vector>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <string.h> // strcmp, strstr

namespace NTEST_NAMESPACE_NAME {

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

#define NTEST_STRING_OF_HELPER(x) #x
#define NTEST_STRING_OF(x) NTEST_STRING_OF_HELPER(x)
#define NTEST_NS_STRING NTEST_STRING_OF(NTEST_NAMESPACE_NAME)

size_t TestBase :: RunTests(std::ostream& log_output, int depth, int argc, char* argv[]) {
    const auto comp = [](const TestBase* left, const TestBase* right) {
        return strcmp(left->getName(), right->getName()) < 0;
    };
    std::vector<std::string> sensitive_patterns;
    std::vector<std::string> insensitive_patterns;
    const auto tolower_string = [](const std::string& s) {
        auto result = s;
        for(auto& ch : result) {
            ch = ::tolower(ch);
        }
        return result;
    };
    const auto should_run = [&sensitive_patterns, &insensitive_patterns, &tolower_string](const char* test_name) -> bool {
        for(auto& patt : sensitive_patterns) {
            if(nullptr != ::strstr(test_name, patt.c_str())) {
                return true;
            }
        }
        if(insensitive_patterns.size()) {
            const auto tmp_insens = tolower_string(std::string(test_name));
            for(auto& patt : insensitive_patterns) {
                if(tmp_insens.find(patt) != std::string::npos) {
                    return true;
                }
            }
        }
        if(sensitive_patterns.size() || insensitive_patterns.size()) {
            return false;
        }
        return true;
    };

    // process args (argc, argv): [-m match_test_name_substring], ..., [-mi match_test_name_substring_case_insensitive], ...
    if(argv && argc) {
        const std::string key_sens("-m");
        const std::string key_insens("-mi");
        for(int i = 1; i < (argc - 1); ++i) {
            const std::string test_str(argv[i]);
            if(key_sens == test_str) {
                sensitive_patterns.emplace_back( std::string(argv[i + 1]) );
                ++i;
            } else if(key_insens == test_str) {
                insensitive_patterns.emplace_back( tolower_string(std::string(argv[i + 1])) );
                ++i;
            }
        }
    }

    auto& tests = GetRunners();
    std::sort(tests.begin(), tests.end(), comp);
    size_t n_failed = 0;
    size_t n_succeeded = 0;
    size_t n_skipped = 0;
    size_t n_crashed = 0;
    size_t n_filtered_out = 0;
    log_output << "ntest" << NTEST_VERSION_STRING << " (compiled with namespace '" << NTEST_NS_STRING
               << "'): Running tests with depth = " << depth << std::endl;
    for(const auto& patt : sensitive_patterns) {
        log_output << "  +sensitive pattern: '" << patt << "';" << std::endl;
    }
    for(const auto& patt : insensitive_patterns) {
        log_output << "  +insensitive pattern: '" << patt << "';" << std::endl;
    }
    double total_time = 0;
    for(auto runner : tests) {
        const auto name = runner->getName();
        if(! should_run(name)) {
            ++n_filtered_out;
            continue; // skip even from listing
        }
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
        log_output << '[' << std::setw(7) << std::left << ResultToStr(result) << "] "
                   << std::setw(9) << std::right << std::fixed << seconds << " : "
                   << name
                   << std::endl;
    }
    log_output << "ntest: Total tests run : " << (n_failed + n_succeeded) << " in " << std::fixed << total_time << " seconds" << std::endl;
    log_output << "ntest: Succeeded : " << n_succeeded << std::endl;
    log_output << "ntest: Skipped : " << n_skipped << std::endl;
    log_output << "ntest: Failed : " << n_failed;
    if(n_crashed) {
        log_output << " (crashed: " << n_crashed << ")";
    }
    log_output << std::endl;
    if(n_filtered_out) {
        log_output << "ntest: Filtered Out: " << n_filtered_out << std::endl;
    }
    return n_failed;
}

#undef NTEST_STRING_OF_HELPER
#undef NTEST_STRING_OF
#undef NTEST_NS_STRING

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
