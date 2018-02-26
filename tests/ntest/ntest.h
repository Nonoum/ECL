#pragma once
#include <iostream>
#include <cstdint>

#define NTEST_VERSION_STRING "2.3.1"

// allow to override namespace to have ability of having multiple thirdparties with ntest easily launched within single project
#ifndef NTEST_NAMESPACE_NAME
    #define NTEST_NAMESPACE_NAME ntest
#endif

namespace NTEST_NAMESPACE_NAME {

// a stub to force adding semicolons after macro
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
    Result run(std::ostream& log, int depth);
    const char* getName() const;
    uint64_t getDurationMicroseconds() const;
    bool isFailed() const { return result == FAIL; };

    static const char* ResultToStr(Result result);
    static int BoundVMinMax(int v, int min, int max); // returns value 'v' bounded to [min..max]. min <= max
    static size_t RunTests(std::ostream& log_output, int depth); // returns amount of failed tests

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

#define NTEST_SUPPRESS_UNUSED (void)log; (void)depth; NTEST_NAMESPACE_NAME::ntest_noop()

#define NTEST_REQUIRE_DEPTH_ABOVE(value) if(depth <= (value)) { skip(); return; } NTEST_NAMESPACE_NAME::ntest_noop()

#define NTEST(test_name) \
    class test_name : public NTEST_NAMESPACE_NAME::TestBase { \
    public: \
        test_name() : NTEST_NAMESPACE_NAME::TestBase(#test_name) { \
            PushRunner(this); \
        }; \
    protected: \
        void runInternal(std::ostream& log, int depth) override; \
    }; \
    static test_name test_name##_instance; \
    void test_name :: runInternal(std::ostream& log, int depth)
