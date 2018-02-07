#include "../ECL_ZeroEater.h"
#include "../ECL_ZeroDevourer.h"
#include "ntest/ntest.h"

#include <vector>

static const uint8_t ECL_test_perf_data_byte_mask = 0x0F;
static const int ECL_test_perf_data_block_size = 50000;
static const int ECL_test_perf_data_repeats = 2000;

NTEST(test_perf_ZeroDevourer_compressor) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    const auto src_size = ECL_test_perf_data_block_size;
    src.resize(src_size);
    for(int j = 0; j < src_size; ++j) {
        src[j] = rand() & ECL_test_perf_data_byte_mask;
    }
    auto enough_size = ECL_ZERO_DEVOURER_GET_BOUND(src_size);
    tmp.resize(enough_size);
    tmp_output.resize(src_size);
    ECL_usize comp_size;
    for(int i = 0; i < ECL_test_perf_data_repeats; ++i) {
        comp_size = ECL_ZeroDevourer_Compress(src.data(), src_size, tmp.data(), enough_size);
    }
    approve(src_size == ECL_ZeroDevourer_Decompress(tmp.data(), comp_size, tmp_output.data(), src_size));
    approve(0 == memcmp(src.data(), tmp_output.data(), src_size));
}

NTEST(test_perf_ZeroDevourer_decompressor) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    const auto src_size = ECL_test_perf_data_block_size;
    src.resize(src_size);
    for(int j = 0; j < src_size; ++j) {
        src[j] = rand() & ECL_test_perf_data_byte_mask;
    }
    auto enough_size = ECL_ZERO_DEVOURER_GET_BOUND(src_size);
    tmp.resize(enough_size);
    tmp_output.resize(src_size);
    auto comp_size = ECL_ZeroDevourer_Compress(src.data(), src_size, tmp.data(), enough_size);
    for(int i = 0; i < ECL_test_perf_data_repeats; ++i) {
        approve(src_size == ECL_ZeroDevourer_Decompress(tmp.data(), comp_size, tmp_output.data(), src_size));
    }
    approve(0 == memcmp(src.data(), tmp_output.data(), src_size));
}

NTEST(test_perf_ZeroEater_compressor) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    const auto src_size = ECL_test_perf_data_block_size;
    src.resize(src_size);
    for(int j = 0; j < src_size; ++j) {
        src[j] = rand() & ECL_test_perf_data_byte_mask;
    }
    auto enough_size = ECL_ZERO_EATER_GET_BOUND(src_size);
    tmp.resize(enough_size);
    tmp_output.resize(src_size);
    ECL_usize comp_size;
    for(int i = 0; i < ECL_test_perf_data_repeats; ++i) {
        comp_size = ECL_ZeroEater_Compress(src.data(), src_size, tmp.data(), enough_size);
    }
    approve(src_size == ECL_ZeroEater_Decompress(tmp.data(), comp_size, tmp_output.data(), src_size));
    approve(0 == memcmp(src.data(), tmp_output.data(), src_size));
}

NTEST(test_perf_ZeroEater_decompressor) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    const auto src_size = ECL_test_perf_data_block_size;
    src.resize(src_size);
    for(int j = 0; j < src_size; ++j) {
        src[j] = rand() & ECL_test_perf_data_byte_mask;
    }
    auto enough_size = ECL_ZERO_EATER_GET_BOUND(src_size);
    tmp.resize(enough_size);
    tmp_output.resize(src_size);
    auto comp_size = ECL_ZeroEater_Compress(src.data(), src_size, tmp.data(), enough_size);
    for(int i = 0; i < ECL_test_perf_data_repeats; ++i) {
        approve(src_size == ECL_ZeroEater_Decompress(tmp.data(), comp_size, tmp_output.data(), src_size));
    }
    approve(0 == memcmp(src.data(), tmp_output.data(), src_size));
}
