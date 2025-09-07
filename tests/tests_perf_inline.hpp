#include "../ECL_ZeroEater.h"
#include "../ECL_ZeroDevourer.h"
#include "../ECL_Huff8.h"
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

NTEST(test_perf_Huff8_ULM_compressor) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    const auto src_size = ECL_test_perf_data_block_size;
    src.resize(src_size);
    for(int j = 0; j < src_size; ++j) {
        src[j] = rand() & ECL_test_perf_data_byte_mask;
    }
    // analyze/compress buffers
    uint16_t buf512[256];
    uint32_t buf1024[256];
    uint8_t buf256[256];
    uint8_t buf768[768];
    // decompress buffer
    uint16_t buf1024_u16[512];
    //
    auto src_data = (const uint8_t*)src.data();
    ECL_TEST_ASSERT(src_data);
    ECL_TEST_ASSERT(src_size);

    const auto enough_size = ECL_HUFF8_GET_BOUND(src_size);
    ECL_TEST_MAGIC_RESIZE(tmp, enough_size);

    uint32_t csize = 0;
    for(int i = 0; i < ECL_test_perf_data_repeats; ++i) {
        csize = ECL_Huff8_Compress16_ULM_Raw(src_data, src_size, 1, buf1024, buf256, buf768, tmp.data(), enough_size);
    }
    const auto a2k_size = ECL_Huff8_Analyze16_ULM(src_data, src_size, 1, buf1024, buf256, buf768);
    const auto a2k5_size = ECL_Huff8_Analyze16_ULM_2k5(src_data, src_size, 1, buf1024, buf256, buf768, buf512);
    const auto comp_size = (csize + 7) / 8; // compressed size in bytes rounded up

    ECL_TEST_COMPARE(csize, a2k_size);
    ECL_TEST_COMPARE(csize, a2k5_size);

    tmp_output.resize(src_size);
    auto consumed_size = ECL_Huff8_Decompress_Raw(tmp.data(), comp_size, buf1024_u16, tmp_output.data(), src_size, 1);
    ECL_TEST_COMPARE(consumed_size, comp_size);
    ECL_TEST_ASSERT(0 == memcmp(src_data, tmp_output.data(), src_size));
    ECL_TEST_MAGIC_VALIDATE(tmp);
}

NTEST(test_perf_Huff8_ULM_decompressor) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    const auto src_size = ECL_test_perf_data_block_size;
    src.resize(src_size);
    for(int j = 0; j < src_size; ++j) {
        src[j] = rand() & ECL_test_perf_data_byte_mask;
    }
    // analyze/compress buffers
    uint16_t buf512[256];
    uint32_t buf1024[256];
    uint8_t buf256[256];
    uint8_t buf768[768];
    // decompress buffer
    uint16_t buf1024_u16[512];
    //
    auto src_data = (const uint8_t*)src.data();
    ECL_TEST_ASSERT(src_data);
    ECL_TEST_ASSERT(src_size);

    const auto enough_size = ECL_HUFF8_GET_BOUND(src_size);
    ECL_TEST_MAGIC_RESIZE(tmp, enough_size);

    const auto csize = ECL_Huff8_Compress16_ULM_Raw(src_data, src_size, 1, buf1024, buf256, buf768, tmp.data(), enough_size);
    const auto a2k_size = ECL_Huff8_Analyze16_ULM(src_data, src_size, 1, buf1024, buf256, buf768);
    const auto a2k5_size = ECL_Huff8_Analyze16_ULM_2k5(src_data, src_size, 1, buf1024, buf256, buf768, buf512);
    const auto comp_size = (csize + 7) / 8; // compressed size in bytes rounded up

    ECL_TEST_COMPARE(csize, a2k_size);
    ECL_TEST_COMPARE(csize, a2k5_size);

    tmp_output.resize(src_size);
    ECL_usize consumed_size = 0;
    for(int i = 0; i < ECL_test_perf_data_repeats; ++i) {
        consumed_size = ECL_Huff8_Decompress_Raw(tmp.data(), comp_size, buf1024_u16, tmp_output.data(), src_size, 1);
    }
    ECL_TEST_COMPARE(consumed_size, comp_size);
    ECL_TEST_ASSERT(0 == memcmp(src_data, tmp_output.data(), src_size));
    ECL_TEST_MAGIC_VALIDATE(tmp);
}
