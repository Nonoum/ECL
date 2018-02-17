#pragma once
// ntest version 2.0
#include <iostream>
#include <cstdint>

namespace ntest {

inline void ntest_noop() {}

uint64_t GetTimeMicroseconds();

class TestBase {
public:
    enum Result {
        INIT,
        SUCCESS,
        SKIP,
        FAIL,
        CRASH,
    };

    TestBase(const char* _name);
    Result run(std::ostream& log, int verbosity);
    const char* getName() const;
    uint64_t getDurationMicroseconds() const;
    bool isFailed() const { return result == FAIL; };

    static const char* ResultToStr(Result result);
    static int BoundVMinMax(int v, int min, int max); // returns value 'v' bounded to [min..max]. min <= max
    static size_t RunTests(std::ostream& log_output, int verbosity); // returns amount of failed tests

protected:
    static void PushRunner(TestBase*);
    bool hasntFailed() const;

    void skip();
    bool approve(bool condition); // don't use 'assert' name to not conflict with C-library. returns whether failed now first time
    virtual void runInternal(std::ostream&, int) = 0;

private:
    const char* name;
    Result result;
    uint64_t time_mcs;
};

} //end ntest

#define NTEST_SUPPRESS_UNUSED (void)log; (void)verbosity; ntest::ntest_noop()

#define NTEST(test_name) \
    class test_name : public ntest::TestBase { \
    public: \
        test_name() : ntest::TestBase(#test_name) { \
            PushRunner(this); \
        }; \
    protected: \
        void runInternal(std::ostream& log, int verbosity) override; \
    }; \
    static test_name test_name##_instance; \
    void test_name :: runInternal(std::ostream& log, int verbosity)
