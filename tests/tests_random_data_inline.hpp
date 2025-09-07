#include "../ECL_ZeroEater.h"
#include "../ECL_ZeroDevourer.h"
#include "../ECL_NanoLZ.h"
#include "../ECL_Huff8.h"
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
    std::vector<uint8_t> tmp_compressed, tmp_compressed_alternative;
    std::vector<uint8_t> tmp_output;
    const int n_sets = 100 * (BoundVMinMax(depth + 10, 0, 1010) + 1);
    const int max_size = 2000;
    const int min_size = 1;
    const uint8_t masks[] = {0x3F, 0x07, 0x03, 0x01};

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

    src.reserve(max_size);
    for(int i = 0; i < n_sets; ++i) {
        const ECL_usize src_size = (rand() % (max_size - min_size)) + min_size;
        src.clear();
        src.resize(src_size);
        for(ECL_usize j = 0; j < src_size; ++j) {
            src[j] = rand();
        }
        const auto src_data = (const uint8_t*)src.data();

        //// copypasted part for huff8 tests >>>
        ECL_TEST_ASSERT(src_data);
        ECL_TEST_ASSERT(src_size);

        const auto enough_size = ECL_HUFF8_GET_BOUND(src_size);
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

        tmp_output.resize(src_size);
        auto consumed_size = ECL_Huff8_Decompress_Raw(tmp_compressed.data(), comp_size, buf1024_u16.data(), tmp_output.data(), src_size, 1);
        ECL_TEST_COMPARE(consumed_size, comp_size);
        ECL_TEST_ASSERT(0 == memcmp(src_data, tmp_output.data(), src_size));
        //// copypasted part for huff8 tests <<<
    }
}
