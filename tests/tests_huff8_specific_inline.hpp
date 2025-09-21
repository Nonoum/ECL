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

NTEST(test_Huff8_depth_edges) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp_compressed, tmp_compressed_alternative;
    std::vector<uint8_t> tmp_output;

    // analyze/compress buffers - declared, set magic outside of loops to minimize reallocations. magic should be checked after each non-const access
    std::vector<uint16_t> buf512_u16;
    std::vector<uint32_t> buf1024_u32;
    std::vector<uint8_t> buf256_u8;
    std::vector<uint8_t> buf768_u8;
    ECL_TEST_MAGIC_U16_RESIZE(buf512_u16, 256);
    ECL_TEST_MAGIC_U32_RESIZE(buf1024_u32, 256);
    ECL_TEST_MAGIC_RESIZE(buf256_u8, 256);
    ECL_TEST_MAGIC_RESIZE(buf768_u8, 768);
    // extra compress* API
    std::vector<uint8_t> buf512_u8;
    std::vector<uint16_t> buf768_u16;
    std::vector<uint8_t> buf32_u8;
    std::vector<uint16_t> buf800_u16;
    std::vector<uint16_t> buf536_u16;
    ECL_TEST_MAGIC_RESIZE(buf512_u8, 512);
    ECL_TEST_MAGIC_U16_RESIZE(buf768_u16, (768/2));
    ECL_TEST_MAGIC_RESIZE(buf32_u8, 32);
    ECL_TEST_MAGIC_U16_RESIZE(buf800_u16, (800/2));
    ECL_TEST_MAGIC_U16_RESIZE(buf536_u16, (536/2));
    // decompress buffer
    std::vector<uint16_t> buf1024_u16;
    ECL_TEST_MAGIC_U16_RESIZE(buf1024_u16, 512);

    // setup
    src.reserve(0x10000);

    auto append_x_n = [&src](uint8_t x, uint16_t n) {
        for(uint16_t i = 0; i < n; ++i) {
            src.push_back(x);
        }
    };
    int64_t sum_pp = 2; append_x_n(0, 1);
    int64_t sum_p = 3; append_x_n(1, 1);
    int64_t sum = 3; append_x_n(2, 1);
    uint16_t tree_depth = 2;
    uint8_t next_code = 3;

    while(src.size() < 0xFFFF) {
        const auto n_new = sum_pp + 1; // adding this amount of bytes having new exact code will result in increasing depth by 1
        //
        for(bool advance_depth : {false, true}) {
            if(! advance_depth) {
                append_x_n(next_code, n_new - 1); // before the edge
            } else {
                append_x_n(next_code, 1); // over the edge
                ++next_code;
                ++tree_depth; // expected depth
                sum = sum_p + n_new;
                sum_pp = sum_p;
                sum_p = sum;
            }
            bool truncated = false;
            if(src.size() > 0xFFFF) {
                src.resize(0xFFFF); // truncate to maximum supported size, this will reduce depth
                truncated = true;
            }
            const ECL_usize src_size = src.size();
            const auto src_data = (const uint8_t*)src.data();

            //// copypasted part for huff8 tests >>>
            ECL_TEST_ASSERT(src_data);
            ECL_TEST_ASSERT(src_size);

            auto enough_size = ECL_HUFF8_GET_BOUND(src_size);
            if((sizeof(ECL_usize) == 2) && (enough_size > 0xFFFF)) {
                enough_size = 0xFFFF; // support running tests in 16 bits here where it overflows but we're sure it will compress correctly
            }
            ECL_TEST_MAGIC_RESIZE(tmp_compressed, enough_size);

            const auto csize = ECL_Huff8_Compress16_ULM_Raw(src_data, src_size, 1, buf1024_u32.data(), buf256_u8.data(), buf768_u8.data(), tmp_compressed.data(), enough_size);
            ECL_TEST_MAGIC_U32_VALIDATE(buf1024_u32);
            ECL_TEST_MAGIC_VALIDATE(buf256_u8);
            ECL_TEST_MAGIC_VALIDATE(buf768_u8);
            ECL_TEST_MAGIC_VALIDATE(tmp_compressed);
            const auto comp_size = (csize + 7) / 8; // compressed size in bytes rounded up

            { // extra API tests
                const auto a2k_size = ECL_Huff8_Analyze16_ULM(src_data, src_size, 1, buf1024_u32.data(), buf256_u8.data(), buf768_u8.data());
                ECL_TEST_MAGIC_U32_VALIDATE(buf1024_u32);
                ECL_TEST_MAGIC_VALIDATE(buf256_u8);
                ECL_TEST_MAGIC_VALIDATE(buf768_u8);

                const auto a2k5_size = ECL_Huff8_Analyze16_ULM_2k5(src_data, src_size, 1, buf1024_u32.data(), buf256_u8.data(), buf768_u8.data(), buf512_u16.data());
                ECL_TEST_MAGIC_U32_VALIDATE(buf1024_u32);
                ECL_TEST_MAGIC_VALIDATE(buf256_u8);
                ECL_TEST_MAGIC_VALIDATE(buf768_u8);

                ECL_TEST_COMPARE(csize, a2k_size);
                ECL_TEST_COMPARE(csize, a2k5_size);

                ECL_Huff8_FillFreqs16(src_data, src_size, 1, buf512_u16.data());
                ECL_TEST_MAGIC_U16_VALIDATE(buf512_u16);

                const auto n_unique = ECL_Huff8_Freqs16ToCTree768(buf512_u16.data(), buf256_u8.data(), buf768_u8.data(), 0);
                ECL_TEST_MAGIC_U16_VALIDATE(buf512_u16);
                ECL_TEST_MAGIC_VALIDATE(buf256_u8);
                ECL_TEST_MAGIC_VALIDATE(buf768_u8);

                const auto csize_tree = ECL_Huff8_EvaluateTreeByN(n_unique);
                ECL_TEST_COMPARE(csize_tree, ECL_Huff8_EvaluateTree(buf768_u8.data()));

                if(n_unique >= 2) { // have meaningful tree (ctree is formed into buf768)
                    // test max depth API, AnalyzeFor API
                    const auto max_depth_ctree = ECL_Huff8_GetMaxDepthCTree768(buf768_u8.data(), buf512_u8.data());
                    ECL_TEST_MAGIC_VALIDATE(buf512_u8);
                    ECL_TEST_ASSERT(max_depth_ctree >= 1);

                    if(! truncated) {
                        ECL_TEST_COMPARE(max_depth_ctree, tree_depth); // validate depth expectations
                    }

                    ECL_Huff8_CTree768ToTSpec1024_ULM(buf768_u8.data(), buf1024_u32.data(), buf256_u8.data());
                    ECL_TEST_MAGIC_U32_VALIDATE(buf1024_u32);
                    ECL_TEST_MAGIC_VALIDATE(buf256_u8);

                    const auto max_depth_tspec1024 = ECL_Huff8_GetMaxDepthTSpec1024(buf1024_u32.data());
                    ECL_TEST_COMPARE(max_depth_ctree, max_depth_tspec1024);

                    const auto csize_tspec1024 = ECL_Huff8_Evaluate16_ForTSpec1024(src_data, src_size, 1, buf1024_u32.data());
                    ECL_TEST_COMPARE(csize, (csize_tree + csize_tspec1024));

                    if(max_depth_ctree <= ECL_HUFF8_TREE_DEPTH_MAX_TSPEC768) {
                        ECL_Huff8_CTree768ToTSpec768(buf768_u8.data(), buf768_u16.data(), buf32_u8.data());
                        ECL_TEST_MAGIC_U16_VALIDATE(buf768_u16);
                        ECL_TEST_MAGIC_VALIDATE(buf32_u8);

                        const auto max_depth_tspec768 = ECL_Huff8_GetMaxDepthTSpec768(buf768_u16.data());
                        ECL_TEST_COMPARE(max_depth_ctree, max_depth_tspec768);

                        const auto csize_tspec768 = ECL_Huff8_Evaluate16_ForTSpec768(src_data, src_size, 1, buf768_u16.data());
                        ECL_TEST_COMPARE(csize, (csize_tree + csize_tspec768));
                    }

                    if(max_depth_ctree <= ECL_HUFF8_TREE_DEPTH_MAX_TSPEC512) {
                        ECL_Huff8_CTree768ToTSpec512(buf768_u8.data(), buf512_u16.data(), buf32_u8.data());
                        ECL_TEST_MAGIC_U16_VALIDATE(buf512_u16);
                        ECL_TEST_MAGIC_VALIDATE(buf32_u8);

                        const auto max_depth_tspec512 = ECL_Huff8_GetMaxDepthTSpec512(buf512_u16.data());
                        ECL_TEST_COMPARE(max_depth_ctree, max_depth_tspec512);

                        const auto csize_tspec512 = ECL_Huff8_Evaluate16_ForTSpec512(src_data, src_size, 1, buf512_u16.data());
                        ECL_TEST_COMPARE(csize, (csize_tree + csize_tspec512));
                    }

                    // test other compress* API - compare with ethalon
                    if(max_depth_ctree <= ECL_HUFF8_TREE_DEPTH_MAX_TSPEC768) {
                        ECL_TEST_MAGIC_RESIZE(tmp_compressed_alternative, enough_size);
                        const auto csize_tspec768 = ECL_Huff8_TryCompress16_TSpec768_Raw(src_data, src_size, 1, buf800_u16.data(), buf768_u8.data(), tmp_compressed_alternative.data(), comp_size);
                        ECL_TEST_MAGIC_U16_VALIDATE(buf800_u16);
                        ECL_TEST_MAGIC_VALIDATE(buf768_u8);
                        ECL_TEST_MAGIC_VALIDATE(tmp_compressed_alternative);

                        ECL_TEST_COMPARE(csize, csize_tspec768);
                        ECL_TEST_COMPARE_CUSTOM(tmp_compressed, tmp_compressed_alternative);
                    }

                    if(max_depth_ctree <= ECL_HUFF8_TREE_DEPTH_MAX_TSPEC512) {
                        ECL_TEST_MAGIC_RESIZE(tmp_compressed_alternative, enough_size);
                        const auto csize_tspec512 = ECL_Huff8_TryCompress16_TSpec512_Raw(src_data, src_size, 1, buf536_u16.data(), buf256_u8.data(), buf768_u8.data(), tmp_compressed_alternative.data(), comp_size);
                        ECL_TEST_MAGIC_U16_VALIDATE(buf536_u16);
                        ECL_TEST_MAGIC_VALIDATE(buf256_u8);
                        ECL_TEST_MAGIC_VALIDATE(buf768_u8);
                        ECL_TEST_MAGIC_VALIDATE(tmp_compressed_alternative);

                        ECL_TEST_COMPARE(csize, csize_tspec512);
                        ECL_TEST_COMPARE_CUSTOM(tmp_compressed, tmp_compressed_alternative);
                    }
                }
            }
            auto corrupt_vec_data = [](std::vector<uint8_t>& v) { for(auto& byte : v) ++byte; };

            tmp_output.resize(src_size);
            auto consumed_size = ECL_Huff8_Decompress_Raw(tmp_compressed.data(), comp_size, buf1024_u16.data(), tmp_output.data(), src_size, 1);
            ECL_TEST_MAGIC_U16_VALIDATE(buf1024_u16);
            ECL_TEST_COMPARE(consumed_size, comp_size);
            ECL_TEST_ASSERT(0 == memcmp(src_data, tmp_output.data(), src_size));

            corrupt_vec_data(tmp_output);
            auto alt_consumed_size = ECL_Huff8_DecompressWithDTable768_Raw(tmp_compressed.data(), comp_size, buf1024_u16.data(), buf768_u16.data(), tmp_output.data(), src_size, 1);
            ECL_TEST_MAGIC_U16_VALIDATE(buf1024_u16);
            ECL_TEST_MAGIC_U16_VALIDATE(buf768_u16);
            ECL_TEST_COMPARE(alt_consumed_size, comp_size);
            ECL_TEST_ASSERT(0 == memcmp(src_data, tmp_output.data(), src_size));
            //// copypasted part for huff8 tests <<<
        }
    }
}
