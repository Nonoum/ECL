#include "../ECL_ZeroEater.h"
#include "../ECL_ZeroDevourer.h"
#include "../ECL_NanoLZ.h"
#include "ntest/ntest.h"

#include <vector>

NTEST(test_ZeroEater_random_data) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    const int n_sets = 100 * (BoundVMinMax(depth + 10, 0, 1010) + 1);
    const int max_size = 60000;
    const int min_size = 1;
    const uint8_t masks[] = {0x3F, 0x07, 0x03, 0x01};
    for(int i = 0; i < n_sets; ++i) {
        const ECL_usize src_size = (rand() % (max_size - min_size)) + min_size;
        src.resize(src_size);
        for(ECL_usize j = 0; j < src_size; ++j) {
            src[j] = rand();
        }

        for(auto mask : masks) {
            for(ECL_usize j = 0; j < src_size; ++j) {
                src[j] &= mask;
            }
            auto enough_size = ECL_ZERO_EATER_GET_BOUND(src_size);
            tmp.resize(enough_size);
            auto comp_size = ECL_ZeroEater_Compress(src.data(), src_size, tmp.data(), enough_size);
            approve(comp_size == ECL_ZeroEater_Compress(src.data(), src_size, nullptr, 0));

            tmp_output.resize(src_size);
            auto decomp_size = ECL_ZeroEater_Decompress(tmp.data(), comp_size, tmp_output.data(), src_size);
            approve(decomp_size == src_size);
            approve(0 == memcmp(src.data(), tmp_output.data(), src_size));
        }
    }
}

NTEST(test_ZeroDevourer_random_data) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    const int n_sets = 100 * (BoundVMinMax(depth + 10, 0, 1010) + 1);
    const int max_size = 60000;
    const int min_size = 1;
    const uint8_t masks[] = {0x3F, 0x07, 0x03, 0x01};
    for(int i = 0; i < n_sets; ++i) {
        const ECL_usize src_size = (rand() % (max_size - min_size)) + min_size;
        src.resize(src_size);
        for(ECL_usize j = 0; j < src_size; ++j) {
            src[j] = rand();
        }

        for(auto mask : masks) {
            for(ECL_usize j = 0; j < src_size; ++j) {
                src[j] &= mask;
            }
            auto enough_size = ECL_ZERO_DEVOURER_GET_BOUND(src_size);
            tmp.resize(enough_size);
            auto comp_size = ECL_ZeroDevourer_Compress(src.data(), src_size, tmp.data(), enough_size);

            tmp_output.resize(src_size);
            auto decomp_size = ECL_ZeroDevourer_Decompress(tmp.data(), comp_size, tmp_output.data(), src_size);
            approve(decomp_size == src_size);
            approve(0 == memcmp(src.data(), tmp_output.data(), src_size));
        }
    }
}

NTEST(test_NanoLZ_slow_random_data) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    const int n_sets = 100 * (BoundVMinMax(depth + 10, 0, 1010) + 1);
    const int max_size = 2000;
    const int min_size = 1;
    const uint8_t masks[] = {0x3F, 0x07, 0x03, 0x01};

    src.reserve(max_size);
    for(int i = 0; i < n_sets; ++i) {
        const ECL_usize src_size = (rand() % (max_size - min_size)) + min_size;
        src.clear();
        src.resize(src_size);
        for(ECL_usize j = 0; j < src_size; ++j) {
            src[j] = rand();
        }

        for(auto mask : masks) {
            for(ECL_usize j = 0; j < src_size; ++j) {
                src[j] &= mask;
            }
            auto enough_size = ECL_NANO_LZ_GET_BOUND(src_size);
            tmp.resize(enough_size);
            for(auto scheme : ECL_NANO_LZ_SCHEMES_ALL) {
                auto comp_size = ECL_NanoLZ_Compress_slow(scheme, src.data(), src_size, tmp.data(), enough_size, -1);
                tmp_output.resize(src_size);
                auto decomp_size = ECL_NanoLZ_Decompress(scheme, tmp.data(), comp_size, tmp_output.data(), src_size);
                approve(decomp_size == src_size);
                approve(0 == memcmp(src.data(), tmp_output.data(), src_size));
            }
        }
    }
}

NTEST(test_NanoLZ_mid1_random_data) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    const int n_sets = 10 * (BoundVMinMax(depth + 10, 0, 1010) + 1);
    const int max_size = 2000;
    const int min_size = 1;
    const uint8_t masks[] = {0x3F, 0x07, 0x03, 0x01};
    const int search_limits[] = {1, 2, 5, 10, -1};
    uint8_t buf_x[256];

    src.reserve(max_size);
    for(int i = 0; i < n_sets; ++i) {
        const ECL_usize src_size = (rand() % (max_size - min_size)) + min_size;
        src.clear();
        src.resize(src_size);
        for(ECL_usize j = 0; j < src_size; ++j) {
            src[j] = rand();
        }

        for(auto mask : masks) {
            for(ECL_usize j = 0; j < src_size; ++j) {
                src[j] &= mask;
            }
            auto enough_size = ECL_NANO_LZ_GET_BOUND(src_size);
            ECL_TEST_MAGIC_RESIZE(tmp, enough_size);
            for(auto limit : search_limits) {
                for(auto scheme : ECL_NANO_LZ_SCHEMES_ALL) {
                    auto comp_size = ECL_NanoLZ_Compress_mid1(scheme, src.data(), src_size, tmp.data(), enough_size, limit, buf_x);
                    tmp_output.resize(src_size);
                    auto decomp_size = ECL_NanoLZ_Decompress(scheme, tmp.data(), comp_size, tmp_output.data(), src_size);
                    approve(decomp_size == src_size);
                    approve(0 == memcmp(src.data(), tmp_output.data(), src_size));
                    ECL_TEST_MAGIC_VALIDATE(tmp);
                }
            }
        }
    }
}

NTEST(test_NanoLZ_mid2_random_data) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    const int n_sets = 10 * (BoundVMinMax(depth + 10, 0, 1010) + 1);
    const int max_size = 2000;
    const int min_size = 1;
    const uint8_t masks[] = {0x3F, 0x07, 0x03, 0x01};
    const int search_limits[] = {1, 2, 5, 10, -1};
    uint8_t buf_x[513];

    src.reserve(max_size);
    for(int i = 0; i < n_sets; ++i) {
        const ECL_usize src_size = (rand() % (max_size - min_size)) + min_size;
        src.clear();
        src.resize(src_size);
        for(ECL_usize j = 0; j < src_size; ++j) {
            src[j] = rand();
        }

        for(auto mask : masks) {
            for(ECL_usize j = 0; j < src_size; ++j) {
                src[j] &= mask;
            }
            auto enough_size = ECL_NANO_LZ_GET_BOUND(src_size);
            ECL_TEST_MAGIC_RESIZE(tmp, enough_size);
            for(auto limit : search_limits) {
                for(auto scheme : ECL_NANO_LZ_SCHEMES_ALL) {
                    auto comp_size = ECL_NanoLZ_Compress_mid2(scheme, src.data(), src_size, tmp.data(), enough_size, limit, buf_x);
                    tmp_output.resize(src_size);
                    auto decomp_size = ECL_NanoLZ_Decompress(scheme, tmp.data(), comp_size, tmp_output.data(), src_size);
                    approve(decomp_size == src_size);
                    approve(0 == memcmp(src.data(), tmp_output.data(), src_size));
                    ECL_TEST_MAGIC_VALIDATE(tmp);
                }
            }
        }
    }
}

NTEST(test_NanoLZ_mid1min_random_data) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    const int n_sets = 10 * (BoundVMinMax(depth + 10, 0, 1010) + 1);
    const int max_size = 2000;
    const int min_size = 1;
    const uint8_t masks[] = {0x3F, 0x07, 0x03, 0x01};
    uint8_t buf_x[256];

    src.reserve(max_size);
    for(int i = 0; i < n_sets; ++i) {
        const ECL_usize src_size = (rand() % (max_size - min_size)) + min_size;
        src.clear();
        src.resize(src_size);
        for(ECL_usize j = 0; j < src_size; ++j) {
            src[j] = rand();
        }

        for(auto mask : masks) {
            for(ECL_usize j = 0; j < src_size; ++j) {
                src[j] &= mask;
            }
            auto enough_size = ECL_NANO_LZ_GET_BOUND(src_size);
            ECL_TEST_MAGIC_RESIZE(tmp, enough_size);
            for(auto scheme : ECL_NANO_LZ_SCHEMES_ALL) {
                auto comp_size = ECL_NanoLZ_Compress_mid1min(scheme, src.data(), src_size, tmp.data(), enough_size, buf_x);
                tmp_output.resize(src_size);
                auto decomp_size = ECL_NanoLZ_Decompress(scheme, tmp.data(), comp_size, tmp_output.data(), src_size);
                approve(decomp_size == src_size);
                approve(0 == memcmp(src.data(), tmp_output.data(), src_size));
                ECL_TEST_MAGIC_VALIDATE(tmp);
            }
        }
    }
}

NTEST(test_NanoLZ_mid2min_random_data) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    const int n_sets = 10 * (BoundVMinMax(depth + 10, 0, 1010) + 1);
    const int max_size = 2000;
    const int min_size = 1;
    const uint8_t masks[] = {0x3F, 0x07, 0x03, 0x01};
    uint8_t buf_x[513];

    src.reserve(max_size);
    for(int i = 0; i < n_sets; ++i) {
        const ECL_usize src_size = (rand() % (max_size - min_size)) + min_size;
        src.clear();
        src.resize(src_size);
        for(ECL_usize j = 0; j < src_size; ++j) {
            src[j] = rand();
        }

        for(auto mask : masks) {
            for(ECL_usize j = 0; j < src_size; ++j) {
                src[j] &= mask;
            }
            auto enough_size = ECL_NANO_LZ_GET_BOUND(src_size);
            ECL_TEST_MAGIC_RESIZE(tmp, enough_size);
            for(auto scheme : ECL_NANO_LZ_SCHEMES_ALL) {
                auto comp_size = ECL_NanoLZ_Compress_mid2min(scheme, src.data(), src_size, tmp.data(), enough_size, buf_x);
                tmp_output.resize(src_size);
                auto decomp_size = ECL_NanoLZ_Decompress(scheme, tmp.data(), comp_size, tmp_output.data(), src_size);
                approve(decomp_size == src_size);
                approve(0 == memcmp(src.data(), tmp_output.data(), src_size));
                ECL_TEST_MAGIC_VALIDATE(tmp);
            }
        }
    }
}

NTEST(test_NanoLZ_auto_random_data) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    const int n_sets = 100 * (BoundVMinMax(depth + 10, 0, 1010) + 1);
    const int max_size = 2000;
    const int min_size = 1;
    const uint8_t masks[] = {0x3F, 0x07, 0x03, 0x01};

    src.reserve(max_size);
    for(int i = 0; i < n_sets; ++i) {
        const ECL_usize src_size = (rand() % (max_size - min_size)) + min_size;
        src.clear();
        src.resize(src_size);
        for(ECL_usize j = 0; j < src_size; ++j) {
            src[j] = rand();
        }

        for(auto mask : masks) {
            for(ECL_usize j = 0; j < src_size; ++j) {
                src[j] &= mask;
            }
            auto enough_size = ECL_NANO_LZ_GET_BOUND(src_size);
            tmp.resize(enough_size);
            for(auto scheme : ECL_NANO_LZ_SCHEMES_ALL) {
                auto comp_size = ECL_NanoLZ_Compress_auto(scheme, src.data(), src_size, tmp.data(), enough_size, -1);
                tmp_output.resize(src_size);
                auto decomp_size = ECL_NanoLZ_Decompress(scheme, tmp.data(), comp_size, tmp_output.data(), src_size);
                approve(decomp_size == src_size);
                approve(0 == memcmp(src.data(), tmp_output.data(), src_size));
            }
        }
    }
}

NTEST(test_Huff8_ULM_random_data) {
    NTEST_SUPPRESS_UNUSED;
    std::vector<uint8_t> src;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> tmp_output;
    const int n_sets = 100 * (BoundVMinMax(depth + 10, 0, 1010) + 1);
    const int max_size = 2000;
    const int min_size = 1;
    const uint8_t masks[] = {0x3F, 0x07, 0x03, 0x01};

    // analyze/compress buffers
    uint16_t buf512[256];
    uint32_t buf1024[256];
    uint8_t buf256[256];
    uint8_t buf768[768];
    // decompress buffer
    uint8_t buf1025[1025];

    src.reserve(max_size);
    for(int i = 0; i < n_sets; ++i) {
        const ECL_usize src_size = (rand() % (max_size - min_size)) + min_size;
        src.clear();
        src.resize(src_size);
        for(ECL_usize j = 0; j < src_size; ++j) {
            src[j] = rand();
        }

        auto src_data = (const uint8_t*)src.data();
        ECL_TEST_ASSERT(src_data);
        ECL_TEST_ASSERT(src_size);

        const auto enough_size = ECL_HUFF8_GET_BOUND(src_size);
        ECL_TEST_MAGIC_RESIZE(tmp, enough_size);

        const auto csize = ECL_Huff8_Compress16_ULM_Raw(src_data, src_size, 1, buf1024, buf256, buf768, tmp.data(), enough_size);
        const auto a2k_size = ECL_Huff8_Analyze16_ULM(src_data, src_size, 1, buf1024, buf256, buf768);
        const auto a2k5_size = ECL_Huff8_Analyze16_ULM_2k5(src_data, src_size, 1, buf1024, buf256, buf768, buf512);
        const auto comp_size = (csize + 7) / 8;

        ECL_TEST_COMPARE(csize, a2k_size);
        ECL_TEST_COMPARE(csize, a2k5_size);

        tmp_output.resize(src_size);
        auto decomp_size = ECL_Huff8_Decompress_Raw(tmp.data(), comp_size, buf1025, tmp_output.data(), src_size, 1);
        ECL_TEST_COMPARE(decomp_size, src_size);
        ECL_TEST_ASSERT(0 == memcmp(src_data, tmp_output.data(), src_size));
        ECL_TEST_MAGIC_VALIDATE(tmp);
    }
}
