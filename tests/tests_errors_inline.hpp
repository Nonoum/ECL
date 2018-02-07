#include "../ECL_ZeroEater.h"
#include "../ECL_ZeroDevourer.h"
#include "../ECL_NanoLZ.h"
#include "ntest/ntest.h"

#include <vector>

NTEST(test_ZeroEater_cut_stream) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    const uint8_t masks[] = {0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01};

    const ECL_usize src_size = 2000;
    src.resize(src_size);
    for(ECL_usize j = 0; j < src_size; ++j) {
        src[j] = rand();
    }
    for(auto mask : masks) {
        for(ECL_usize j = 0; j < src_size; ++j) {
            src[j] &= mask;
        }
        const auto enough_size = ECL_ZERO_EATER_GET_BOUND(src_size);
        ECL_TEST_MAGIC_RESIZE(tmp, enough_size);
        auto comp_size = ECL_ZeroEater_Compress(src.data(), src_size, tmp.data(), enough_size);
        tmp_output.resize(src_size);
        auto decomp_size = ECL_ZeroEater_Decompress(tmp.data(), comp_size, tmp_output.data(), src_size);
        approve(decomp_size == src_size);
        approve(0 == memcmp(src.data(), tmp_output.data(), src_size));
        ECL_TEST_MAGIC_VALIDATE(tmp);
        // test cut stream - decompression has to fail correctly
        for(ECL_usize i = 1; i <= comp_size; ++i) {
            auto decomp_size = ECL_ZeroEater_Decompress(tmp.data(), comp_size - i, tmp_output.data(), src_size);
            approve(decomp_size < src_size);
        }
    }
}

NTEST(test_ZeroDevourer_cut_stream) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    const uint8_t masks[] = {0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01};

    const ECL_usize src_size = 2000;
    src.resize(src_size);
    for(ECL_usize j = 0; j < src_size; ++j) {
        src[j] = rand();
    }
    for(auto mask : masks) {
        for(ECL_usize j = 0; j < src_size; ++j) {
            src[j] &= mask;
        }
        const auto enough_size = ECL_ZERO_DEVOURER_GET_BOUND(src_size);
        ECL_TEST_MAGIC_RESIZE(tmp, enough_size);
        auto comp_size = ECL_ZeroDevourer_Compress(src.data(), src_size, tmp.data(), enough_size);
        tmp_output.resize(src_size);
        auto decomp_size = ECL_ZeroDevourer_Decompress(tmp.data(), comp_size, tmp_output.data(), src_size);
        approve(decomp_size == src_size);
        approve(0 == memcmp(src.data(), tmp_output.data(), src_size));
        ECL_TEST_MAGIC_VALIDATE(tmp);
        // test cut stream - decompression has to fail correctly
        for(ECL_usize i = 1; i <= comp_size; ++i) {
            auto decomp_size = ECL_ZeroDevourer_Decompress(tmp.data(), comp_size - i, tmp_output.data(), src_size);
            approve(! decomp_size);
        }
    }
}

NTEST(test_NanoLZ_cut_stream) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    const uint8_t masks[] = {0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01};
    const int search_limits[] = {1, 2, 5, 10, -1};

    const ECL_usize src_size = 2000;
    src.resize(src_size);
    for(ECL_usize j = 0; j < src_size; ++j) {
        src[j] = rand();
    }
    ECL_NanoLZ_FastParams fp;
    ECL_NanoLZ_FastParams_Alloc1(&fp, 11);
    for(auto mask : masks) {
        for(ECL_usize j = 0; j < src_size; ++j) {
            src[j] &= mask;
        }
        const auto enough_size = ECL_NANO_LZ_GET_BOUND(src_size);
        ECL_TEST_MAGIC_RESIZE(tmp, enough_size);
        for(auto limit : search_limits) {
            for(auto scheme : ECL_NANO_LZ_SCHEMES_ALL) {
                auto comp_size = ECL_NanoLZ_Compress_fast1(scheme, src.data(), src_size, tmp.data(), enough_size, limit, &fp);
                tmp_output.resize(src_size);
                auto decomp_size = ECL_NanoLZ_Decompress(scheme, tmp.data(), comp_size, tmp_output.data(), src_size);
                approve(decomp_size == src_size);
                approve(0 == memcmp(src.data(), tmp_output.data(), src_size));
                ECL_TEST_MAGIC_VALIDATE(tmp);
                // test cut stream - decompression has to fail correctly
                for(ECL_usize i = 1; i <= comp_size; ++i) {
                    auto decomp_size = ECL_NanoLZ_Decompress(scheme, tmp.data(), comp_size - i, tmp_output.data(), src_size);
                    approve(! decomp_size);
                }
            }
        }
    }
    ECL_NanoLZ_FastParams_Destroy(&fp);
}

NTEST(test_ZeroDevourer_insufficient_dst_compr) {
    NTEST_SUPPRESS_UNUSED;
    // ZeroEater compressor behaves differently for tested scenario so no such test provided
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    const uint8_t masks[] = {0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01};
    const ECL_usize src_size = 8000;

    src.resize(src_size);
    for(ECL_usize j = 0; j < src_size; ++j) {
        src[j] = rand();
    }
    for(auto mask : masks) {
        for(ECL_usize j = 0; j < src_size; ++j) {
            src[j] &= mask;
        }
        const auto enough_size = ECL_ZERO_DEVOURER_GET_BOUND(src_size);
        tmp.resize(enough_size);
        auto comp_size = ECL_ZeroDevourer_Compress(src.data(), src_size, tmp.data(), enough_size);
        tmp_output.resize(src_size);
        auto decomp_size = ECL_ZeroDevourer_Decompress(tmp.data(), comp_size, tmp_output.data(), src_size);
        approve(decomp_size == src_size);
        approve(0 == memcmp(src.data(), tmp_output.data(), src_size));
        // test with insufficient output buffer - compression has to fail correctly
        for(ECL_usize i = 1; i <= comp_size; ++i) {
            const auto output_size = comp_size - i;
            ECL_TEST_MAGIC_RESIZE(tmp, output_size);
            auto sz = ECL_ZeroDevourer_Compress(src.data(), src_size, tmp.data(), output_size);
            approve(! sz);
            ECL_TEST_MAGIC_VALIDATE(tmp);
        }
    }
}

NTEST(test_NanoLZ_insufficient_dst_compr) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    const uint8_t masks[] = {0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01};
    const int search_limits[] = {1, 5, -1};
    const ECL_usize src_size = 2000;

    src.resize(src_size);
    for(ECL_usize j = 0; j < src_size; ++j) {
        src[j] = rand();
    }
    ECL_NanoLZ_FastParams fp;
    ECL_NanoLZ_FastParams_Alloc1(&fp, 11);
    for(auto mask : masks) {
        for(ECL_usize j = 0; j < src_size; ++j) {
            src[j] &= mask;
        }
        const auto enough_size = ECL_NANO_LZ_GET_BOUND(src_size);
        tmp.resize(enough_size);
        for(auto limit : search_limits) {
            for(auto scheme : ECL_NANO_LZ_SCHEMES_ALL) {
                auto comp_size = ECL_NanoLZ_Compress_fast1(scheme, src.data(), src_size, tmp.data(), enough_size, limit, &fp);
                tmp_output.resize(src_size);
                auto decomp_size = ECL_NanoLZ_Decompress(scheme, tmp.data(), comp_size, tmp_output.data(), src_size);
                approve(decomp_size == src_size);
                approve(0 == memcmp(src.data(), tmp_output.data(), src_size));
                // test with insufficient output buffer - compression has to fail correctly
                for(ECL_usize i = 1; i <= comp_size; ++i) {
                    const auto output_size = comp_size - i;
                    ECL_TEST_MAGIC_RESIZE(tmp, output_size);
                    auto sz = ECL_NanoLZ_Compress_fast1(scheme, src.data(), src_size, tmp.data(), output_size, limit, &fp);
                    approve(! sz);
                    ECL_TEST_MAGIC_VALIDATE(tmp);
                }
            }
        }
    }
    ECL_NanoLZ_FastParams_Destroy(&fp);
}

NTEST(test_ZeroEater_insufficient_dst_decompr) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    const uint8_t masks[] = {0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01};
    const ECL_usize src_size = 8000;

    src.resize(src_size);
    for(ECL_usize j = 0; j < src_size; ++j) {
        src[j] = rand();
    }
    for(auto mask : masks) {
        for(ECL_usize j = 0; j < src_size; ++j) {
            src[j] &= mask;
        }
        const auto enough_size = ECL_ZERO_EATER_GET_BOUND(src_size);
        tmp.resize(enough_size);
        auto comp_size = ECL_ZeroEater_Compress(src.data(), src_size, tmp.data(), enough_size);
        tmp_output.resize(src_size);
        auto decomp_size = ECL_ZeroEater_Decompress(tmp.data(), comp_size, tmp_output.data(), src_size);
        approve(decomp_size == src_size);
        approve(0 == memcmp(src.data(), tmp_output.data(), src_size));
        // test with insufficient output buffer - compression has to fail correctly
        for(ECL_usize i = 1; i <= src_size; ++i) {
            const auto output_size = src_size - i;
            ECL_TEST_MAGIC_RESIZE(tmp_output, output_size);
            auto sz = ECL_ZeroEater_Decompress(tmp.data(), comp_size, tmp_output.data(), output_size);
            approve(! sz);
            ECL_TEST_MAGIC_VALIDATE(tmp_output);
        }
    }
}

NTEST(test_ZeroDevourer_insufficient_dst_decompr) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    const uint8_t masks[] = {0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01};
    const ECL_usize src_size = 8000;

    src.resize(src_size);
    for(ECL_usize j = 0; j < src_size; ++j) {
        src[j] = rand();
    }
    for(auto mask : masks) {
        for(ECL_usize j = 0; j < src_size; ++j) {
            src[j] &= mask;
        }
        const auto enough_size = ECL_ZERO_DEVOURER_GET_BOUND(src_size);
        tmp.resize(enough_size);
        auto comp_size = ECL_ZeroDevourer_Compress(src.data(), src_size, tmp.data(), enough_size);
        tmp_output.resize(src_size);
        auto decomp_size = ECL_ZeroDevourer_Decompress(tmp.data(), comp_size, tmp_output.data(), src_size);
        approve(decomp_size == src_size);
        approve(0 == memcmp(src.data(), tmp_output.data(), src_size));
        // test with insufficient output buffer - compression has to fail correctly
        for(ECL_usize i = 1; i <= src_size; ++i) {
            const auto output_size = src_size - i;
            ECL_TEST_MAGIC_RESIZE(tmp_output, output_size);
            auto sz = ECL_ZeroDevourer_Decompress(tmp.data(), comp_size, tmp_output.data(), output_size);
            approve(! sz);
            ECL_TEST_MAGIC_VALIDATE(tmp_output);
        }
    }
}

NTEST(test_NanoLZ_insufficient_dst_decompr) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    const uint8_t masks[] = {0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01};
    const int search_limits[] = {1, 2, 5, -1};
    const ECL_usize src_size = 2000;

    src.resize(src_size);
    for(ECL_usize j = 0; j < src_size; ++j) {
        src[j] = rand();
    }
    ECL_NanoLZ_FastParams fp;
    ECL_NanoLZ_FastParams_Alloc1(&fp, 11);
    for(auto mask : masks) {
        for(ECL_usize j = 0; j < src_size; ++j) {
            src[j] &= mask;
        }
        const auto enough_size = ECL_NANO_LZ_GET_BOUND(src_size);
        tmp.resize(enough_size);
        for(auto limit : search_limits) {
            for(auto scheme : ECL_NANO_LZ_SCHEMES_ALL) {
                auto comp_size = ECL_NanoLZ_Compress_fast1(scheme, src.data(), src_size, tmp.data(), enough_size, limit, &fp);
                tmp_output.resize(src_size);
                auto decomp_size = ECL_NanoLZ_Decompress(scheme, tmp.data(), comp_size, tmp_output.data(), src_size);
                approve(decomp_size == src_size);
                approve(0 == memcmp(src.data(), tmp_output.data(), src_size));
                // test with insufficient output buffer - compression has to fail correctly
                for(ECL_usize i = 1; i <= src_size; ++i) {
                    const auto output_size = src_size - i;
                    ECL_TEST_MAGIC_RESIZE(tmp_output, output_size);
                    auto sz = ECL_NanoLZ_Decompress(scheme, tmp.data(), comp_size, tmp_output.data(), output_size);
                    approve(! sz);
                    ECL_TEST_MAGIC_VALIDATE(tmp_output);
                }
            }
        }
    }
    ECL_NanoLZ_FastParams_Destroy(&fp);
}
