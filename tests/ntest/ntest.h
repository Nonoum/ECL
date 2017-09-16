#pragma once
#include <vector>
#include <iostream>
#include <cstdint>

namespace ntest {

class TestBase {
    const char* name;
    enum {
        INIT,
        SUCCESS,
        FAIL
    } result;
    uint64_t tacts;
public:
    TestBase(const char* _name);
    bool run(std::ostream& log);
    const char* getName() const;
    uint64_t getTacts() const;
    bool isFailed() const { return result == FAIL; };

    static size_t RunTests(std::ostream& log_output); // returns count of fails
protected:
    static void PushRunner(TestBase*);
    bool hasntFailed() const;

    void approve(bool condition); // don't use 'assert' name to not conflict with C-library
    virtual void runInternal(std::ostream&) = 0;
};

} //end ntest

#define NTEST(test_name) \
    class test_name : public ntest::TestBase { \
    public: \
        test_name() : ntest::TestBase(#test_name) { \
            PushRunner(this); \
        }; \
    protected: \
        void runInternal(std::ostream& log) override; \
    }; \
    static test_name test_name##_instance; \
    void test_name :: runInternal(std::ostream& log)
