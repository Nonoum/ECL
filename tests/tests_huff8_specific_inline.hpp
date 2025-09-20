#include "../ECL_Huff8.h"
#include "ntest/ntest.h"

#include <vector>
#include <algorithm>

#ifdef ECL_BUILD_AS_C
extern "C"
#endif
void ECL_Huff8_Aux_QSort16(uint8_t* codes, uint16_t* values, unsigned size, uint8_t* buf512);

#ifdef ECL_BUILD_AS_C
extern "C"
#endif
void ECL_Huff8_Aux_HSort16(uint8_t* codes, uint16_t* values, unsigned size, uint8_t* buf768);

NTEST(test_Huff8_Aux_QSort16) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> codes, backup_codes;
    std::vector<uint16_t> values, backup_values;
    codes.reserve(256);
    values.reserve(256);
    std::vector<uint8_t> working_buf;
    const size_t working_buf_size = 512;

    static const auto huff_bubble_sort = [](uint8_t* codes, uint16_t* values, int size) {
        int i, j;
        uint8_t db;
        uint16_t dw;
        for(i = 1; i < size; ++i) {
            db = codes[i];
            dw = values[i];
            for(j = i - 1; (j >= 0) && values[j] < dw; --j) {
                codes[j+1] = codes[j];
                values[j+1] = values[j];
            }
            values[j+1] = dw;
            codes[j+1] = db;
        }
    };

    const auto setup = [&codes, &values](std::initializer_list< std::pair<uint8_t, uint16_t> > init_list) -> bool {
        assert(init_list.size());
        assert(init_list.size() <= 256);

        bool valid = true;
        uint8_t used_map[256];
        memset(used_map, 0, 256);

        codes.clear();
        values.clear();
        for(auto& pair : init_list) {
            if(used_map[pair.first]) {
                valid = false;
            }
            used_map[pair.first] = 1;
            codes.push_back(pair.first);
            values.push_back(pair.second);
        }
        return valid;
    };
    const auto backup_and_sort = [&codes, &values, &backup_codes, &backup_values, &working_buf]() {
        backup_codes = codes;
        backup_values = values;
        ECL_Huff8_Aux_QSort16(codes.data(), values.data(), (unsigned)codes.size(), working_buf.data());
    };
    const auto compare = [&codes, &values, &backup_codes, &backup_values]() -> bool {
        for(size_t i = 1; i < values.size(); ++i) {
            if(values[i] > values[i-1]) {
                return false;
            }
        }
        huff_bubble_sort(backup_codes.data(), backup_values.data(), (int)backup_codes.size());
        //
        if(0 != memcmp(codes.data(), backup_codes.data(), codes.size())) {
            // simple check can fail - do deep check considering permutations for equal values
            for(size_t i = 0; i < backup_codes.size(); ++i) {
                const auto code = backup_codes[i];
                const auto value = backup_values[i];
                const auto rng = std::equal_range(values.begin(), values.end(), value, [](auto lhs, auto rhs) { return lhs > rhs; });
                if(rng.first == rng.second) {
                    return false;
                }
                bool found = false;
                for(auto it = rng.first; it < rng.second; ++it) {
                    auto idx = it - values.begin();
                    if(codes[idx] == code) {
                        found = true;
                        continue;
                    }
                }
                if(! found) {
                    return false;
                }
            }
        }
        if(0 != memcmp(values.data(), backup_values.data(), (values.size() * sizeof(uint16_t)))) {
            return false;
        }
        return true;
    };

    const auto setup_equal = [&codes, &values](uint16_t value, size_t n) {
        codes.clear();
        values.clear();
        for(size_t i = 0; i < n; ++i) {
            codes.push_back(uint8_t(i));
            values.push_back(value);
        }
    };
    const auto setup_revsorted = [&codes, &values](size_t n, int shift) {
        codes.clear();
        values.clear();
        for(size_t i = 0; i < n; ++i) {
            codes.push_back(uint8_t(i));
            values.push_back(uint16_t(n - i));
        }
        if(shift > 0) {
            std::rotate(values.begin(), values.begin() + shift, values.end());
        } else if(shift < 0) {
            std::rotate(values.begin(), values.begin() + values.size() + shift, values.end());
        }
    };
    {
        ECL_TEST_MAGIC_RESIZE(working_buf, working_buf_size);
        ECL_TEST_ASSERT(setup({
            {1  , 100}, {2  , 999}, {3  , 326}, {4  , 47 }, {5  , 4  }, {6  , 5  }, {7  , 2  }, {8  , 1  }, {9  , 199}, {10 , 19 },
            {11 , 405}, {12 , 333}, {13 , 332}, {14 , 331}, {15 , 330}, {16 , 239}, {17 , 328}, {18 , 335}, {19 , 336}, {20 , 337},
            {21 , 111}, {22 , 112}, {23 , 113}, {24 , 114}, {25 , 115}, {26 , 116}, {27 , 117}, {28 , 118}, {29 , 119}, {30 , 120},
            {31 , 222}, {32 , 223}, {33 , 224}, {34 , 225}, {35 , 226}, {36 , 227}, {37 , 228}, {38 , 229}, {39 , 230}, {40 , 231},
        }));
        backup_and_sort();
        ECL_TEST_ASSERT(compare());
        ECL_TEST_MAGIC_VALIDATE(working_buf);
        //ECL_TEST_MAGIC_LOG_TOUCHED_SIZE(working_buf, "qsort buf touched: ");
    }
    {
        ECL_TEST_MAGIC_RESIZE(working_buf, working_buf_size);
        setup_equal(1, 255);
        backup_and_sort();
        ECL_TEST_ASSERT(compare());
        ECL_TEST_MAGIC_VALIDATE(working_buf);
        //ECL_TEST_MAGIC_LOG_TOUCHED_SIZE(working_buf, "qsort buf touched: ");
    }
    {
        ECL_TEST_MAGIC_RESIZE(working_buf, working_buf_size);
        setup_equal(1, 256);
        backup_and_sort();
        ECL_TEST_ASSERT(compare());
        ECL_TEST_MAGIC_VALIDATE(working_buf);
        //ECL_TEST_MAGIC_LOG_TOUCHED_SIZE(working_buf, "qsort buf touched: ");
    }
    {
        ECL_TEST_MAGIC_RESIZE(working_buf, working_buf_size);
        setup_equal(2, 256);
        values[100] = 3;
        backup_and_sort();
        ECL_TEST_ASSERT(compare());
        ECL_TEST_MAGIC_VALIDATE(working_buf);
        //ECL_TEST_MAGIC_LOG_TOUCHED_SIZE(working_buf, "qsort buf touched: ");
    }
    {
        ECL_TEST_MAGIC_RESIZE(working_buf, working_buf_size);
        setup_equal(2, 256);
        values[100] = 1;
        backup_and_sort();
        ECL_TEST_ASSERT(compare());
        ECL_TEST_MAGIC_VALIDATE(working_buf);
        //ECL_TEST_MAGIC_LOG_TOUCHED_SIZE(working_buf, "qsort buf touched: ");
    }
    {
        ECL_TEST_MAGIC_RESIZE(working_buf, working_buf_size);
        setup_revsorted(256, 0);
        backup_and_sort();
        ECL_TEST_ASSERT(compare());
        ECL_TEST_MAGIC_VALIDATE(working_buf);
        //ECL_TEST_MAGIC_LOG_TOUCHED_SIZE(working_buf, "qsort buf touched: ");
    }
    {
        ECL_TEST_MAGIC_RESIZE(working_buf, working_buf_size);
        setup_revsorted(256, 100);
        backup_and_sort();
        ECL_TEST_ASSERT(compare());
        ECL_TEST_MAGIC_VALIDATE(working_buf);
        //ECL_TEST_MAGIC_LOG_TOUCHED_SIZE(working_buf, "qsort buf touched: ");
    }
    {
        ECL_TEST_MAGIC_RESIZE(working_buf, working_buf_size);
        setup_revsorted(256, -88);
        backup_and_sort();
        ECL_TEST_ASSERT(compare());
        ECL_TEST_MAGIC_VALIDATE(working_buf);
        //ECL_TEST_MAGIC_LOG_TOUCHED_SIZE(working_buf, "qsort buf touched: ");
    }
    for(int i = -128; i < 128; ++i) {
        ECL_TEST_MAGIC_RESIZE(working_buf, working_buf_size);
        setup_revsorted(256, i);
        backup_and_sort();
        ECL_TEST_ASSERT(compare());
        ECL_TEST_MAGIC_VALIDATE(working_buf);
        //ECL_TEST_MAGIC_LOG_TOUCHED_SIZE(working_buf, "qsort buf touched: ");
    }
}

NTEST(test_Huff8_Aux_HSort16) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> codes, backup_codes;
    std::vector<uint16_t> values, backup_values;
    codes.reserve(256);
    values.reserve(256);
    std::vector<uint8_t> working_buf;
    const size_t working_buf_size = 768;

    static const auto huff_bubble_sort = [](uint8_t* codes, uint16_t* values, int size) {
        int i, j;
        uint8_t db;
        uint16_t dw;
        for(i = 1; i < size; ++i) {
            db = codes[i];
            dw = values[i];
            for(j = i - 1; (j >= 0) && values[j] < dw; --j) {
                codes[j+1] = codes[j];
                values[j+1] = values[j];
            }
            values[j+1] = dw;
            codes[j+1] = db;
        }
    };

    const auto setup = [&codes, &values](std::initializer_list< std::pair<uint8_t, uint16_t> > init_list) -> bool {
        assert(init_list.size());
        assert(init_list.size() <= 256);

        bool valid = true;
        uint8_t used_map[256];
        memset(used_map, 0, 256);

        codes.clear();
        values.clear();
        for(auto& pair : init_list) {
            if(used_map[pair.first]) {
                valid = false;
            }
            used_map[pair.first] = 1;
            codes.push_back(pair.first);
            values.push_back(pair.second);
        }
        return valid;
    };
    const auto backup_and_sort = [&codes, &values, &backup_codes, &backup_values, &working_buf]() {
        backup_codes = codes;
        backup_values = values;
        ECL_Huff8_Aux_HSort16(codes.data(), values.data(), (unsigned)codes.size(), working_buf.data());
    };
    const auto compare = [&codes, &values, &backup_codes, &backup_values]() -> bool {
        for(size_t i = 1; i < values.size(); ++i) {
            if(values[i] > values[i-1]) {
                return false;
            }
        }
        huff_bubble_sort(backup_codes.data(), backup_values.data(), (int)backup_codes.size());
        //
        if(0 != memcmp(codes.data(), backup_codes.data(), codes.size())) {
            // simple check can fail - do deep check considering permutations for equal values
            for(size_t i = 0; i < backup_codes.size(); ++i) {
                const auto code = backup_codes[i];
                const auto value = backup_values[i];
                const auto rng = std::equal_range(values.begin(), values.end(), value, [](auto lhs, auto rhs) { return lhs > rhs; });
                if(rng.first == rng.second) {
                    return false;
                }
                bool found = false;
                for(auto it = rng.first; it < rng.second; ++it) {
                    auto idx = it - values.begin();
                    if(codes[idx] == code) {
                        found = true;
                        continue;
                    }
                }
                if(! found) {
                    return false;
                }
            }
        }
        if(0 != memcmp(values.data(), backup_values.data(), (values.size() * sizeof(uint16_t)))) {
            return false;
        }
        return true;
    };

    {
        ECL_TEST_MAGIC_RESIZE(working_buf, working_buf_size);
        ECL_TEST_ASSERT(setup({
            {1, 5}, {2, 5}, {3, 5}, {4, 5},
        }));
        backup_and_sort();
        ECL_TEST_ASSERT(compare());
        ECL_TEST_MAGIC_VALIDATE(working_buf);
    }
    {
        ECL_TEST_MAGIC_RESIZE(working_buf, working_buf_size);
        ECL_TEST_ASSERT(setup({
            {5, 3},   {1, 5}, {2, 5}, {3, 5}, {4, 5},
        }));
        backup_and_sort();
        ECL_TEST_ASSERT(compare());
        ECL_TEST_MAGIC_VALIDATE(working_buf);
    }
    {
        ECL_TEST_MAGIC_RESIZE(working_buf, working_buf_size);
        ECL_TEST_ASSERT(setup({
            {1, 5},   {5, 3},   {2, 5}, {3, 5}, {4, 5},
        }));
        backup_and_sort();
        ECL_TEST_ASSERT(compare());
        ECL_TEST_MAGIC_VALIDATE(working_buf);
    }
    {
        ECL_TEST_MAGIC_RESIZE(working_buf, working_buf_size);
        ECL_TEST_ASSERT(setup({
            {1, 5}, {2, 5},   {5, 3},   {3, 5}, {4, 5},
        }));
        backup_and_sort();
        ECL_TEST_ASSERT(compare());
        ECL_TEST_MAGIC_VALIDATE(working_buf);
    }
    {
        ECL_TEST_MAGIC_RESIZE(working_buf, working_buf_size);
        ECL_TEST_ASSERT(setup({
            {1, 5}, {2, 5}, {3, 5},   {5, 3},   {4, 5},
        }));
        backup_and_sort();
        ECL_TEST_ASSERT(compare());
        ECL_TEST_MAGIC_VALIDATE(working_buf);
    }
    {
        ECL_TEST_MAGIC_RESIZE(working_buf, working_buf_size);
        ECL_TEST_ASSERT(setup({
            {1, 5}, {2, 5}, {3, 5}, {4, 5},   {5, 3},
        }));
        backup_and_sort();
        ECL_TEST_ASSERT(compare());
        ECL_TEST_MAGIC_VALIDATE(working_buf);
    }
    { // generic loop
        ECL_TEST_MAGIC_RESIZE(working_buf, working_buf_size);
        for(int n_unique = 1; n_unique < 200; n_unique += 1) {
            for(int n_max_freq = 1; n_max_freq < 10; ++n_max_freq) {
                codes.clear();
                values.clear();
                for(int i = 0; i < n_unique; ++i) {
                    codes.push_back(uint8_t(i));
                    values.push_back((rand() % n_max_freq) + 1);
                }
                backup_and_sort();
                ECL_TEST_ASSERT(compare());
                ECL_TEST_MAGIC_VALIDATE(working_buf);
            }
        }
    }
}
