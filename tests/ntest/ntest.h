#pragma once
#include <iostream>
#include <cstdint>

#define NTEST_VERSION_STRING "3.1.0"

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
    static size_t RunTests(std::ostream& log_output, int depth, int argc = 0, char* argv[] = nullptr); // returns amount of failed tests

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

} // end namespace ntest

#define NTEST_SUPPRESS_UNUSED (void)log; (void)depth; NTEST_NAMESPACE_NAME::ntest_noop()

#define NTEST_REQUIRE_DEPTH_ABOVE(value) if(depth <= (value)) { skip(); return; } NTEST_NAMESPACE_NAME::ntest_noop()

#define NTEST_ASSERT(expr)                                         \
    {                                                              \
        if(! (bool)(expr)) {                                       \
            if(hasntFailed()) {                                    \
                log << "fail expr:'" #expr "' at file: " << __FILE__ << ", line: " << std::dec << __LINE__ << "; "; \
            }                                                      \
            approve(false);                                        \
        } else {                                                   \
            approve(true);                                         \
        }                                                          \
    }

#define NTEST_ASSERT_EX(expr, commenter_func/* [](auto& log) { ... } */) \
    {                                                              \
        if(! (bool)(expr)) {                                       \
            if(hasntFailed()) {                                    \
                log << "fail expr:'" #expr "' at file: " << __FILE__ << ", line: " << std::dec << __LINE__ << "; "; commenter_func(log); \
            }                                                      \
            approve(false);                                        \
        } else {                                                   \
            approve(true);                                         \
        }                                                          \
    }

// integer and pointer type values (being logged in std::hex and std::dec)
#define NTEST_COMPARE(val1, val2)                                  \
    {                                                              \
        if((val1) != (val2)) {                                     \
            if(hasntFailed()) {                                    \
                log << "fail comp: '"#val1 "' (0x" << std::hex << (val1) << ") != '" #val2 "' (0x" << (val2) \
                    << ") at file: " << __FILE__ << ", line: " << std::dec << __LINE__ << "; "; \
            }                                                      \
            approve(false);                                        \
        } else {                                                   \
            approve(true);                                         \
        }                                                          \
    }

#define NTEST_COMPARE_EX(val1, val2, commenter_func/* [](auto& log) { ... } */) \
    {                                                              \
        if((val1) != (val2)) {                                     \
            if(hasntFailed()) {                                    \
                log << "fail comp: '"#val1 "' (0x" << std::hex << (val1) << ") != '" #val2 "' (0x" << (val2) \
                    << ") at file: " << __FILE__ << ", line: " << std::dec << __LINE__ << "; "; commenter_func(log); \
            }                                                      \
            approve(false);                                        \
        } else {                                                   \
            approve(true);                                         \
        }                                                          \
    }

// non-integer non-pointer type values
#define NTEST_COMPARE_CUSTOM(val1, val2)                           \
    {                                                              \
        if((val1) != (val2)) {                                     \
            if(hasntFailed()) {                                    \
                log << "fail comp: '"#val1 "' != '" #val2 "'"      \
                    << " at file: " << __FILE__ << ", line: " << std::dec << __LINE__ << "; "; \
            }                                                      \
            approve(false);                                        \
        } else {                                                   \
            approve(true);                                         \
        }                                                          \
    }

#define NTEST_COMPARE_CUSTOM_EX(val1, val2, commenter_func/* [](auto& log) { ... } */) \
    {                                                              \
        if((val1) != (val2)) {                                     \
            if(hasntFailed()) {                                    \
                log << "fail comp: '"#val1 "' != '" #val2 "'"      \
                    << " at file: " << __FILE__ << ", line: " << std::dec << __LINE__ << "; "; commenter_func(log); \
            }                                                      \
            approve(false);                                        \
        } else {                                                   \
            approve(true);                                         \
        }                                                          \
    }



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
