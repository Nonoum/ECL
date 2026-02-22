#pragma once
#include <cassert>
#include <chrono>

class HelperTimeMeasurer {
    uint64_t us_sum = 0;
    uint64_t ts_us_last_start = 0;
    bool started = false;

    static uint64_t GetTimeMicroseconds() {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

public:
    HelperTimeMeasurer() = default;
    ~HelperTimeMeasurer() {
        assert(! started);
    }

    void reset() {
        assert(! started);
        *this = {};
    }
    void startResume() { // can ba called any number of times, when stopped
        assert(! started);
        started = true;
        ts_us_last_start = GetTimeMicroseconds();
    }
    void stop() { // stop current sub-session and append it's duration to total sum, can call startResume again afterwards
        assert(started);
        started = false;
        auto ts_us_last_stop = GetTimeMicroseconds();
        auto dur = ts_us_last_stop - ts_us_last_start;
        us_sum += dur;
    }
    uint64_t getTotalUS() const {
        assert((! started) && "expected to be called in non-active state, returns duration accumulated at last 'stop' call");
        return us_sum;
    }
    double getTotalSeconds() const {
        assert((! started) && "expected to be called in non-active state, returns duration accumulated at last 'stop' call");
        return double(us_sum) / 1000000.;
    }
    double calcSpeed_Mb_S(uint64_t n_bytes) const {
        auto seconds = getTotalSeconds();
        return double(n_bytes) / (double(1024*1024) * seconds);
    }
};
