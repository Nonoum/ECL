#include "../ECL_ZeroEater.h"
#include "../ECL_ZeroDevourer.h"
#include "../ECL_NanoLZ.h"
#include "ntest/ntest.h"

#include <vector>
#include <numeric>

struct ECLDatasetRecord {
    const char* ptr;
    int length;
};

typedef std::vector<ECLDatasetRecord> DatasetRecords;
static DatasetRecords& GetDatasetRecords() {
    static DatasetRecords recs;
    return recs;
}

#define ECL_TEST_APPEND_DATASET(the_string) \
    GetDatasetRecords().push_back({the_string, sizeof(the_string) - 1}); \

struct ECLTestDatasetsInitializer {
    ECLTestDatasetsInitializer() {
        // common datasets (basically for Zero-oriented algorithms)
        ECL_TEST_APPEND_DATASET("\x0");
        ECL_TEST_APPEND_DATASET("\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1");
        ECL_TEST_APPEND_DATASET("\x0\x0");
        ECL_TEST_APPEND_DATASET("\x0\x0\x0");
        ECL_TEST_APPEND_DATASET("\x0\x0\x0\x0");
        ECL_TEST_APPEND_DATASET("\x0\x0\x0\x0\x0");
        ECL_TEST_APPEND_DATASET("\x0\x1\x0\x0\x0");
        ECL_TEST_APPEND_DATASET("\x0\x0\x1\x0\x0");
        ECL_TEST_APPEND_DATASET("\x0\x1\x1\x0\x0");
        ECL_TEST_APPEND_DATASET("\x0\x1\x1\x0\x1");
        ECL_TEST_APPEND_DATASET("\x0\x1\x1\x0\x1\x0\x0\x0\x0\x0\x0");
        ECL_TEST_APPEND_DATASET("\x0\x1\x1\x0\x1\x0");
        ECL_TEST_APPEND_DATASET("\x0\x1\x1\x0\x1\x0\x1");
        ECL_TEST_APPEND_DATASET("\x0\x1\x1\x0\x1\x0\x0\x1");
        ECL_TEST_APPEND_DATASET("\x0\x1\x1\x0\x1\x0\x0\x0\x1");
        ECL_TEST_APPEND_DATASET("\x0\x1\x1\x0\x1\x0\x0\x0\x0\x1");
        ECL_TEST_APPEND_DATASET("\x0\x1\x1\x0\x1\x0\x0\x0\x0\x0\x1");
        ECL_TEST_APPEND_DATASET("\x0\x1\x1\x0\x1\x0\x0\x0\x0\x0\x0\x1");
        ECL_TEST_APPEND_DATASET("\x0\x1\x1\x0\x1\x0\x0\x0\x0\x0\x0\x0\x1");
        ECL_TEST_APPEND_DATASET("\x0\x1\x1\x0\x1\x0\x0\x0\x0\x0\x0\x0\x0\x1");
        ECL_TEST_APPEND_DATASET("\x0\x1\x1\x0\x1\x0\x0\x0\x0\x0\x0\x0\x0\x0\x1");
        ECL_TEST_APPEND_DATASET("\x0\x1\x1\x0\x1\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x1");
        ECL_TEST_APPEND_DATASET("\x0\x1\x1\x0\x1\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x1");
        ECL_TEST_APPEND_DATASET("\x0\x1\x1\x0\x1\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x1");
        ECL_TEST_APPEND_DATASET("\x0\x1\x1\x0\x1\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x1\x0");
        ECL_TEST_APPEND_DATASET("\x0\x0\x0\x0\x0\x1\x1\x0\x1\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x1\x0");
        ECL_TEST_APPEND_DATASET("\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x1\x1\x0\x1\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x1\x0");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x1\x0");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x1\x0\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x1\x0\x0\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x0");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x0\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x0\x0\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x0\x0\x1\x0");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x0\x0\x1\x0\x1");
        // LZ datasets
        ECL_TEST_APPEND_DATASET("\x1\x2\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x2\x3\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x2\x3\x4\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x2\x3\x4\x5\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x2\x3\x4\x5\x6\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x2\x3\x4\x5\x6\x7\x1\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x2\x3\x4\x5\x6\x7\x8\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x2\x3\x4\x5\x6\x7\x8\x9\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA\xB\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA\xB\xC\x1\x1\x1\x1");
        // similar 1
        ECL_TEST_APPEND_DATASET("\x1\x1\x2\x3\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x2\x3\x4\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x2\x3\x4\x5\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x2\x3\x4\x5\x6\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x2\x3\x4\x5\x6\x7\x1\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x2\x3\x4\x5\x6\x7\x8\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x2\x3\x4\x5\x6\x7\x8\x9\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA\xB\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA\xB\xC\x1\x1\x1\x1");
        // similar 2
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x2\x3\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x2\x3\x4\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x2\x3\x4\x5\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x2\x3\x4\x5\x6\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x2\x3\x4\x5\x6\x7\x1\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x2\x3\x4\x5\x6\x7\x8\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x2\x3\x4\x5\x6\x7\x8\x9\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA\xB\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA\xB\xC\x1\x1\x1\x1");
        // similar 3
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x1\x2\x3\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x1\x2\x3\x4\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x1\x2\x3\x4\x5\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x1\x2\x3\x4\x5\x6\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x1\x2\x3\x4\x5\x6\x7\x1\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x1\x2\x3\x4\x5\x6\x7\x8\x1\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x1\x2\x3\x4\x5\x6\x7\x8\x9\x1\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA\x1\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA\xB\x1\x1\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x1\x1\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA\xB\xC\x1\x1\x1\x1");
        //
        ECL_TEST_APPEND_DATASET("\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA\xB\xC\x1\x4\x5\x6\x7\x1\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA\xB\xC\x1\x1\x4\x5\x6\x7\x1\x1");
        ECL_TEST_APPEND_DATASET("\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA\xB\xC\x1\x1\x1\x4\x5\x6\x7\x1");
        ECL_TEST_APPEND_DATASET("\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA\xB\xC\x1\x1\x1\x1\x4\x5\x6\x7");
    }
};
ECLTestDatasetsInitializer ECLTestDatasetsInitializer_instance;

#undef ECL_TEST_APPEND_DATASET

NTEST(test_ZeroEater_datasets) {
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    for(auto& rec : GetDatasetRecords()) {
        auto src_data = (const uint8_t*)rec.ptr;
        auto src_size = rec.length;
        approve(src_data);
        approve(src_size);

        auto enough_size = ECL_ZERO_EATER_GET_BOUND(src_size);
        tmp.resize(enough_size);
        auto comp_size = ECL_ZeroEater_Compress(src_data, src_size, tmp.data(), enough_size);
        approve(comp_size == ECL_ZeroEater_Compress(src_data, src_size, nullptr, 0));

        tmp_output.resize(src_size);
        auto decomp_size = ECL_ZeroEater_Decompress(tmp.data(), comp_size, tmp_output.data(), src_size);
        approve(decomp_size == src_size);
        approve(0 == memcmp(src_data, tmp_output.data(), src_size));
    }
}

NTEST(test_ZeroDevourer_datasets) {
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    for(auto& rec : GetDatasetRecords()) {
        auto src_data = (const uint8_t*)rec.ptr;
        auto src_size = rec.length;
        approve(src_data);
        approve(src_size);

        auto enough_size = ECL_ZERO_DEVOURER_GET_BOUND(src_size);
        tmp.resize(enough_size);
        auto comp_size = ECL_ZeroDevourer_Compress(src_data, src_size, tmp.data(), enough_size);

        tmp_output.resize(src_size);
        auto decomp_size = ECL_ZeroDevourer_Decompress(tmp.data(), comp_size, tmp_output.data(), src_size);
        approve(decomp_size == src_size);
        approve(0 == memcmp(src_data, tmp_output.data(), src_size));
    }
}

NTEST(test_NanoLZ_slow_datasets) {
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    for(auto& rec : GetDatasetRecords()) {
        auto src_data = (const uint8_t*)rec.ptr;
        auto src_size = rec.length;
        approve(src_data);
        approve(src_size);

        auto enough_size = ECL_NANO_LZ_GET_BOUND(src_size);
        tmp.resize(enough_size);
        for(auto scheme : ECL_NANO_LZ_SCHEMES_ALL) {
            auto comp_size = ECL_NanoLZ_Compress_slow(scheme, src_data, src_size, tmp.data(), enough_size, -1);
            tmp_output.resize(src_size);
            auto decomp_size = ECL_NanoLZ_Decompress(scheme, tmp.data(), comp_size, tmp_output.data(), src_size);
            approve(decomp_size == src_size);
            approve(0 == memcmp(src_data, tmp_output.data(), src_size));
        }
    }
}

NTEST(test_NanoLZ_fast1_datasets) {
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;

    ECL_NanoLZ_FastParams fp;
    ECL_NanoLZ_FastParams_Alloc1(&fp, 10);
    for(auto& rec : GetDatasetRecords()) {
        auto src_data = (const uint8_t*)rec.ptr;
        auto src_size = rec.length;
        approve(src_data);
        approve(src_size);

        auto enough_size = ECL_NANO_LZ_GET_BOUND(src_size);
        tmp.resize(enough_size);
        for(auto scheme : ECL_NANO_LZ_SCHEMES_ALL) {
            auto comp_size = ECL_NanoLZ_Compress_fast1(scheme, src_data, src_size, tmp.data(), enough_size, -1, &fp);
            tmp_output.resize(src_size);
            auto decomp_size = ECL_NanoLZ_Decompress(scheme, tmp.data(), comp_size, tmp_output.data(), src_size);
            approve(decomp_size == src_size);
            approve(0 == memcmp(src_data, tmp_output.data(), src_size));
        }
    }
    ECL_NanoLZ_FastParams_Destroy(&fp);
}

NTEST(test_NanoLZ_fast2_datasets) {
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;

    ECL_NanoLZ_FastParams fp;
    ECL_NanoLZ_FastParams_Alloc2(&fp, 10);
    for(auto& rec : GetDatasetRecords()) {
        auto src_data = (const uint8_t*)rec.ptr;
        auto src_size = rec.length;
        approve(src_data);
        approve(src_size);

        auto enough_size = ECL_NANO_LZ_GET_BOUND(src_size);
        tmp.resize(enough_size);
        for(auto scheme : ECL_NANO_LZ_SCHEMES_ALL) {
            auto comp_size = ECL_NanoLZ_Compress_fast2(scheme, src_data, src_size, tmp.data(), enough_size, -1, &fp);
            tmp_output.resize(src_size);
            auto decomp_size = ECL_NanoLZ_Decompress(scheme, tmp.data(), comp_size, tmp_output.data(), src_size);
            approve(decomp_size == src_size);
            approve(0 == memcmp(src_data, tmp_output.data(), src_size));
        }
    }
    ECL_NanoLZ_FastParams_Destroy(&fp);
}

NTEST(test_NanoLZ_mid1_datasets) {
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    uint8_t buf_x[256];
    const int search_limits[] = {1, 2, 5, 10, -1};

    for(auto& rec : GetDatasetRecords()) {
        auto src_data = (const uint8_t*)rec.ptr;
        auto src_size = rec.length;
        approve(src_data);
        approve(src_size);

        auto enough_size = ECL_NANO_LZ_GET_BOUND(src_size);
        ECL_TEST_MAGIC_RESIZE(tmp, enough_size);
        for(auto limit : search_limits) {
            for(auto scheme : ECL_NANO_LZ_SCHEMES_ALL) {
                auto comp_size = ECL_NanoLZ_Compress_mid1(scheme, src_data, src_size, tmp.data(), enough_size, limit, buf_x);
                tmp_output.resize(src_size);
                auto decomp_size = ECL_NanoLZ_Decompress(scheme, tmp.data(), comp_size, tmp_output.data(), src_size);
                approve(decomp_size == src_size);
                approve(0 == memcmp(src_data, tmp_output.data(), src_size));
                ECL_TEST_MAGIC_VALIDATE(tmp);
            }
        }
    }
}

NTEST(test_NanoLZ_mid2_datasets) {
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    uint8_t buf_x[512];
    const int search_limits[] = {1, 2, 5, 10, -1};

    for(auto& rec : GetDatasetRecords()) {
        auto src_data = (const uint8_t*)rec.ptr;
        auto src_size = rec.length;
        approve(src_data);
        approve(src_size);

        auto enough_size = ECL_NANO_LZ_GET_BOUND(src_size);
        ECL_TEST_MAGIC_RESIZE(tmp, enough_size);
        for(auto limit : search_limits) {
            for(auto scheme : ECL_NANO_LZ_SCHEMES_ALL) {
                auto comp_size = ECL_NanoLZ_Compress_mid2(scheme, src_data, src_size, tmp.data(), enough_size, limit, buf_x);
                tmp_output.resize(src_size);
                auto decomp_size = ECL_NanoLZ_Decompress(scheme, tmp.data(), comp_size, tmp_output.data(), src_size);
                approve(decomp_size == src_size);
                approve(0 == memcmp(src_data, tmp_output.data(), src_size));
                ECL_TEST_MAGIC_VALIDATE(tmp);
            }
        }
    }
}

ECL_usize ECL_Test_NanoLZ_CompressWith(ECL_NanoLZ_Scheme scheme, const std::vector<uint8_t>& src, std::vector<uint8_t>& preallocated_output, int mode, ECL_NanoLZ_FastParams& preallocated_params) {
    ECL_usize comp_size = 0;
    if(mode == 0) {
        comp_size = ECL_NanoLZ_Compress_slow(scheme,
            src.data(), src.size(),
            preallocated_output.data(), preallocated_output.size(),
            -1);
    } else if(mode == 1) {
        comp_size = ECL_NanoLZ_Compress_fast1(scheme,
            src.data(), src.size(),
            preallocated_output.data(), preallocated_output.size(),
            -1, &preallocated_params);
    } else if(mode == 2) {
        comp_size = ECL_NanoLZ_Compress_fast2(scheme,
            src.data(), src.size(),
            preallocated_output.data(), preallocated_output.size(),
            -1, &preallocated_params);
    }
    preallocated_output.resize(comp_size);
    return comp_size;
}

bool ECL_Test_NanoLZ_OnLinearGenericData(std::ostream& log, int mode, ECL_NanoLZ_FastParams& preallocated_params) {
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;

    const int prefixes[] = {0, 1, 2, 10};
    const int suffixes[] = {0, 1, 2, 10};
    const int counts[] = {2, 3, 4, 5, 7, 8, 11};
    const int new_ones[] = {0, 1, 2, 3, 4, 6, 8, 10, 18};
    const int min_dist = 0;
    const int max_dist = 800;

    const int n_reserve = 1000;
    src.reserve(n_reserve);
    tmp.reserve(n_reserve);
    tmp_output.reserve(n_reserve);
    for(auto prefix_size : prefixes) {
        for(auto suffix_size : suffixes) {
            for(auto match_size : counts) {
                for(auto n_new : new_ones) {
                    for(int dist = min_dist; dist < max_dist; ++dist) {
                        // generate data
                        const auto src_size = prefix_size + match_size*2 + dist + suffix_size;
                        src.resize(src_size);
                        auto next_random_value = match_size;
                        auto next_ptr = src.data();
                        std::iota(next_ptr, next_ptr + prefix_size, next_random_value); // --- prefix
                        next_ptr += prefix_size;
                        next_random_value += prefix_size;
                        std::iota(next_ptr, next_ptr + match_size, 0); // --- first string
                        next_ptr += match_size;
                        { // fill the gap
                            auto n_generate = dist;
                            if(n_new < dist) { // first, add a block that will hopefully get glued
                                auto n_repeated = dist - n_new;
                                std::fill(next_ptr, next_ptr + n_repeated, next_random_value);
                                next_ptr += n_repeated;
                                ++next_random_value;
                                n_generate -= n_repeated;
                            }
                            std::iota(next_ptr, next_ptr + n_generate, next_random_value);
                            next_ptr += n_generate;
                            next_random_value += n_generate;
                        }
                        std::iota(next_ptr, next_ptr + match_size, 0); // --- second string - match
                        next_ptr += match_size;
                        std::iota(next_ptr, next_ptr + suffix_size, next_random_value); // --- suffix
                        next_ptr += suffix_size;
                        if(next_ptr != (src.data() + src_size)) {
                            return false;
                        }
                        if(next_random_value > 255) {
                            return false; // oops
                        }

                        // test
                        tmp.resize(ECL_NANO_LZ_GET_BOUND(src_size));
                        for(auto scheme : ECL_NANO_LZ_SCHEMES_ALL) {
                            ECL_usize comp_size = ECL_Test_NanoLZ_CompressWith(scheme, src, tmp, mode, preallocated_params);
                            if(! comp_size) {
                                return false;
                            }
                            tmp_output.resize(src_size);
                            auto decomp_size = ECL_NanoLZ_Decompress(scheme, tmp.data(), comp_size, tmp_output.data(), src_size);
                            if(decomp_size != src_size) {
                                return false;
                            }
                            if(memcmp(src.data(), tmp_output.data(), src_size)) {
                                return false;
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}

NTEST(test_NanoLZ_fast1_generic_datasets) { // use only for fast1 version, as it's optimal for target data sizes
    ECL_NanoLZ_FastParams fp;
    ECL_NanoLZ_FastParams_Alloc1(&fp, 10);
    approve(ECL_Test_NanoLZ_OnLinearGenericData(log, 1, fp));
    ECL_NanoLZ_FastParams_Destroy(&fp);
}

bool ECL_Test_NanoLZ_AllCompressorsAreEqual(const std::vector<uint8_t>& src, std::ostream& log) {
    // checks that all compression modes with maximum depth of search end up with equal binary output
    using Vec = std::vector<uint8_t>;
    ECL_NanoLZ_FastParams fp1;
    ECL_NanoLZ_FastParams_Alloc1(&fp1, 15);
    ECL_NanoLZ_FastParams fp2;
    ECL_NanoLZ_FastParams_Alloc2(&fp2, 15);
    bool result = true;

    for(auto scheme : ECL_NANO_LZ_SCHEMES_ALL) {
        Vec last;
        for(int mode = 0; mode < 3; ++mode) {
            Vec tmp;
            tmp.resize(ECL_NANO_LZ_GET_BOUND(src.size()));
            if(mode == 1) {
                ECL_Test_NanoLZ_CompressWith(scheme, src, tmp, mode, fp1);
            } else if(mode == 2) {
                ECL_Test_NanoLZ_CompressWith(scheme, src, tmp, mode, fp2);
            } else {
                ECL_Test_NanoLZ_CompressWith(scheme, src, tmp, mode, fp1);
            }
            if((! last.empty()) && (last != tmp)) {
                log << " mode=" << mode << " mismatches previous. " << last.size() << " -- " << tmp.size() << std::endl;
                result = false;
                break;
            }
            last = std::move(tmp);
        }
        if(! result) {
            break;
        }
        Vec decompressed;
        decompressed.resize(src.size());
        auto decomp_size = ECL_NanoLZ_Decompress(scheme, last.data(), last.size(), decompressed.data(), decompressed.size());
        if(decomp_size != decompressed.size()) {
            result = false;
            break;
        }
        if(src != decompressed) {
            result = false;
            break;
        }
    }

    ECL_NanoLZ_FastParams_Destroy(&fp1);
    ECL_NanoLZ_FastParams_Destroy(&fp2);
    return result;
}

NTEST(test_NanoLZ_check_modes_equal_result) {
    std::vector<uint8_t> src;
    const int n_sets = 20;
    const int max_size = 10000;
    const int min_size = 1;
    const uint8_t masks[] = {0x3F, 0x07, 0x03, 0x01};

    src.reserve(max_size);
    for(int i = 0; i < n_sets; ++i) {
        const auto src_size = (rand() % (max_size - min_size)) + min_size;
        src.clear();
        src.resize(src_size);
        for(int j = 0; j < src_size; ++j) {
            src[j] = rand();
        }

        for(auto mask : masks) {
            for(int j = 0; j < src_size; ++j) {
                src[j] &= mask;
            }
            approve( ECL_Test_NanoLZ_AllCompressorsAreEqual(src, log) );
        }
    }
}
