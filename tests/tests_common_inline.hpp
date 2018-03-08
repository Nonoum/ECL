#include "../ECL_JH_States.h"
#include "../ECL_utils.h"
#include "ntest/ntest.h"

#include <algorithm>

#define ECL_TEST_E_NEXT_VALUE(value) (((value) * 128) / 127 + 1)

#define ECL_TEST_E_NUMBER_GENERIC(write_func, read_func, eval_func)                         \
    {                                                                                       \
        const uint64_t limit = 0x0FFFFFFFFFFFFFFLL & ((1ULL << (ECL_SIZE_TYPE_BITS_COUNT - 1)) - 1ULL); \
        ECL_JH_WState wstate;                                                               \
        ECL_JH_RState rstate;                                                               \
        ECL_usize n_bits = 0;                                                               \
        for(uint64_t value = 0; value < limit; value = ECL_TEST_E_NEXT_VALUE(value)) {      \
            n_bits += eval_func(value);                                                     \
        }                                                                                   \
        auto n_bytes = (n_bits / 8) + 1;                                                    \
        auto data = (uint8_t*)malloc(n_bytes);                                              \
        ECL_TEST_ASSERT(data);                                                              \
        const auto n_repeats = (BoundVMinMax(depth + 10, 0, 1010) + 2);                     \
        for(int i = 0; i < n_repeats; ++i) {                                                \
            ECL_JH_WInit(&wstate, data, n_bytes, 0);                                        \
            for(uint64_t value = 0; value < limit; value = ECL_TEST_E_NEXT_VALUE(value)) {  \
                write_func(&wstate, value);                                                 \
            }                                                                               \
            ECL_TEST_ASSERT(wstate.is_valid);                                               \
            ECL_JH_RInit(&rstate, data, n_bytes, 0);                                        \
            for(uint64_t value = 0; value < limit; value = ECL_TEST_E_NEXT_VALUE(value)) {  \
                auto retrieved = read_func(&rstate);                                        \
                ECL_TEST_COMPARE((ECL_usize)value, retrieved);                              \
            }                                                                               \
            ECL_TEST_ASSERT(rstate.is_valid);                                               \
            ECL_TEST_ASSERT(wstate.byte == rstate.byte);                                    \
            ECL_TEST_ASSERT(wstate.next == rstate.next);                                    \
            ECL_TEST_ASSERT(wstate.n_bits == rstate.n_bits);                                \
        }                                                                                   \
        free(data);                                                                         \
    }

NTEST(test_number_E7_helpers) {
    NTEST_SUPPRESS_UNUSED;
    {
        const uint64_t limit = 0x0FFFFFFFFFFFFFFLL & ((1ULL << (ECL_SIZE_TYPE_BITS_COUNT - 1)) - 1ULL);
        ECL_usize n_bits = 0;
        for(uint64_t value = 0; value < limit; value = ECL_TEST_E_NEXT_VALUE(value)) {
            n_bits += ECL_Evaluate_E7(value);
        }
        const auto n_bytes = (n_bits / 8) + 1;
        auto data = (uint8_t*)malloc(n_bytes);
        const auto data_end = data + n_bytes;
        ECL_TEST_ASSERT(data);
        const auto n_repeats = (BoundVMinMax(depth + 10, 0, 1010) + 2);
        for(int i = 0; i < n_repeats; ++i) {
            uint8_t* next_wr = data;
            for(uint64_t value = 0; value < limit; value = ECL_TEST_E_NEXT_VALUE(value)) {
                next_wr = ECL_Helper_WriteE7(next_wr, data_end - next_wr, value);
                ECL_TEST_ASSERT(next_wr);
            }
            auto finished_wr = next_wr;
            // read back
            const uint8_t* next_rd = data;
            for(uint64_t value = 0; value < limit; value = ECL_TEST_E_NEXT_VALUE(value)) {
                ECL_usize retrieved = 0;
                next_rd = ECL_Helper_ReadE7(next_rd, data_end - next_rd, &retrieved);
                ECL_TEST_ASSERT(next_rd);
                ECL_TEST_COMPARE((ECL_usize)value, retrieved);
            }
            auto finished_rd = next_rd;
            //
            ECL_TEST_ASSERT(finished_wr <= data_end);
            ECL_TEST_COMPARE((int)finished_wr, (int)finished_rd);
        }
        free(data);
    }
}

NTEST(test_number_E7) {
    NTEST_SUPPRESS_UNUSED;
    NTEST_REQUIRE_DEPTH_ABOVE(0);
    ECL_TEST_E_NUMBER_GENERIC(ECL_JH_Write_E7, ECL_JH_Read_E7, ECL_Evaluate_E7);
}

NTEST(test_number_E6) {
    NTEST_SUPPRESS_UNUSED;
    NTEST_REQUIRE_DEPTH_ABOVE(0);
    ECL_TEST_E_NUMBER_GENERIC(ECL_JH_Write_E6, ECL_JH_Read_E6, ECL_Evaluate_E6);
}

NTEST(test_number_E5) {
    NTEST_SUPPRESS_UNUSED;
    NTEST_REQUIRE_DEPTH_ABOVE(-1);
    ECL_TEST_E_NUMBER_GENERIC(ECL_JH_Write_E5, ECL_JH_Read_E5, ECL_Evaluate_E5);
}

NTEST(test_number_E4) {
    NTEST_SUPPRESS_UNUSED;
    ECL_TEST_E_NUMBER_GENERIC(ECL_JH_Write_E4, ECL_JH_Read_E4, ECL_Evaluate_E4);
}

NTEST(test_number_E3) {
    NTEST_REQUIRE_DEPTH_ABOVE(-1);
    NTEST_SUPPRESS_UNUSED;
    ECL_TEST_E_NUMBER_GENERIC(ECL_JH_Write_E3, ECL_JH_Read_E3, ECL_Evaluate_E3);
}

NTEST(test_number_E2) {
    NTEST_SUPPRESS_UNUSED;
    ECL_TEST_E_NUMBER_GENERIC(ECL_JH_Write_E2, ECL_JH_Read_E2, ECL_Evaluate_E2);
}

NTEST(test_number_E7E4) {
    NTEST_SUPPRESS_UNUSED;
    NTEST_REQUIRE_DEPTH_ABOVE(0);
    ECL_TEST_E_NUMBER_GENERIC(ECL_JH_Write_E7E4, ECL_JH_Read_E7E4, ECL_Evaluate_E7E4);
}

NTEST(test_number_E6E3) {
    NTEST_SUPPRESS_UNUSED;
    ECL_TEST_E_NUMBER_GENERIC(ECL_JH_Write_E6E3, ECL_JH_Read_E6E3, ECL_Evaluate_E6E3);
}

NTEST(test_number_E4E5) {
    NTEST_SUPPRESS_UNUSED;
    NTEST_REQUIRE_DEPTH_ABOVE(-1);
    ECL_TEST_E_NUMBER_GENERIC(ECL_JH_Write_E4E5, ECL_JH_Read_E4E5, ECL_Evaluate_E4E5);
}

#undef ECL_TEST_E_NEXT_VALUE

NTEST(test_JH_generic) {
    NTEST_SUPPRESS_UNUSED;
    uint8_t bit_counts[] = {1,2,3,4,4,5,6,7,8};
    uint8_t data[5];
    ECL_JH_WState wstate;
    ECL_JH_RState rstate;
    do {
        ECL_JH_WInit(&wstate, data, 5, 0);
        ECL_JH_RInit(&rstate, data, 5, 0);
        uint8_t value = 0;
        for(auto n_bits : bit_counts) {
            ECL_JH_Write(&wstate, value, n_bits);
            value = ~value;
        }
        value = 0;
        for(auto n_bits : bit_counts) {
            auto retrieved = ECL_JH_Read(&rstate, n_bits);
            uint8_t expected = value & ((1 << n_bits) - 1);
            ECL_TEST_ASSERT(retrieved == expected);
            value = ~value;
        }
        ECL_TEST_ASSERT(wstate.is_valid);
        ECL_TEST_ASSERT(rstate.is_valid);
        ECL_TEST_ASSERT(wstate.byte == rstate.byte);
        ECL_TEST_ASSERT(wstate.next == rstate.next);
        ECL_TEST_ASSERT(wstate.n_bits == rstate.n_bits);
    } while(std::next_permutation(std::begin(bit_counts), std::end(bit_counts)));
}

NTEST(test_JH_writing_failures) {
    NTEST_SUPPRESS_UNUSED;
    uint8_t data[6];
    ECL_JH_WState state;
    {
        data[1] = 0xFF;
        ECL_JH_WInit(&state, data, 1, 0);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 8);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 1);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_JH_Write(&state, 0, 1);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_TEST_ASSERT(data[1] == uint8_t(0xFF));
    }
    {
        data[1] = 0xFF;
        ECL_JH_WInit(&state, data, 1, 0);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 7);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 1);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 1);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_JH_Write(&state, 0, 1);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_TEST_ASSERT(data[1] == uint8_t(0xFF));
    }
    {
        data[2] = 0xFF;
        ECL_JH_WInit(&state, data, 2, 0);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 8);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 8);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 1);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_JH_Write(&state, 0, 1);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_TEST_ASSERT(data[2] == uint8_t(0xFF));
    }
    {
        data[2] = 0xFF;
        ECL_JH_WInit(&state, data, 2, 0);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 7);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 7);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 2);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 1);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_JH_Write(&state, 0, 1);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_TEST_ASSERT(data[2] == uint8_t(0xFF));
    }
    {
        data[2] = 0xFF;
        ECL_JH_WInit(&state, data, 2, 0);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 7);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 7);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 3);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_JH_Write(&state, 0, 3);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_TEST_ASSERT(data[2] == uint8_t(0xFF));
    }
    {
        data[2] = 0xFF;
        ECL_JH_WInit(&state, data, 2, 0);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 7);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 7);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 8);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_JH_Write(&state, 0, 8);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_TEST_ASSERT(data[2] == uint8_t(0xFF));
    }
    {
        data[5] = 0xFF;
        ECL_JH_WInit(&state, data, 5, 0);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 5);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_WJump(&state, 4);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 2);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 2);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_JH_Write(&state, 0, 2);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_TEST_ASSERT(data[5] == uint8_t(0xFF));
    }
    {
        data[5] = 0xFF;
        ECL_JH_WInit(&state, data, 5, 0);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 5);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_WJump(&state, 3);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 7);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 4);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_Write(&state, 0, 1);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_JH_Write(&state, 0, 1);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_TEST_ASSERT(data[5] == uint8_t(0xFF));
    }
    {
        data[5] = 0xFF;
        ECL_JH_WInit(&state, data, 5, 0);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_WJump(&state, 4);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_WJump(&state, 1);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_JH_Write(&state, 0, 8);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_TEST_ASSERT(data[5] == uint8_t(0xFF));
    }
    {
        data[5] = 0xFF;
        ECL_JH_WInit(&state, data, 5, 0);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_WJump(&state, 5);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_JH_WJump(&state, 1);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_JH_Write(&state, 0, 8);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_TEST_ASSERT(data[5] == uint8_t(0xFF));
    }
}

NTEST(test_JH_reading_failures) {
    NTEST_SUPPRESS_UNUSED;
    uint8_t data[6];
    ECL_JH_RState state;
    {
        memset(data, 0, sizeof(data));
        data[1] = 0xFF;
        ECL_JH_RInit(&state, data, 1, 0);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 8));
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 1));
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 1));
        ECL_TEST_ASSERT(! state.is_valid);
    }
    {
        memset(data, 0, sizeof(data));
        data[1] = 0xFF;
        ECL_JH_RInit(&state, data, 1, 0);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 7));
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 1));
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 1));
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 1));
        ECL_TEST_ASSERT(! state.is_valid);
    }
    {
        memset(data, 0, sizeof(data));
        data[2] = 0xFF;
        ECL_JH_RInit(&state, data, 2, 0);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 8));
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 8));
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 1));
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 1));
        ECL_TEST_ASSERT(! state.is_valid);
    }
    {
        memset(data, 0, sizeof(data));
        data[2] = 0xFF;
        ECL_JH_RInit(&state, data, 2, 0);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 7));
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 7));
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 2));
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 1));
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 1));
        ECL_TEST_ASSERT(! state.is_valid);
    }
    {
        memset(data, 0, sizeof(data));
        data[2] = 0xFF;
        ECL_JH_RInit(&state, data, 2, 0);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 7));
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 7));
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 3));
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 3));
        ECL_TEST_ASSERT(! state.is_valid);
    }
    {
        memset(data, 0, sizeof(data));
        data[2] = 0xFF;
        ECL_JH_RInit(&state, data, 2, 0);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 7));
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 7));
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 8));
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 8));
        ECL_TEST_ASSERT(! state.is_valid);
    }
    {
        memset(data, 0, sizeof(data));
        data[5] = 0xFF;
        ECL_JH_RInit(&state, data, 5, 0);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 5));
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_RJump(&state, 4);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 2));
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 2));
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 2));
        ECL_TEST_ASSERT(! state.is_valid);
    }
    {
        memset(data, 0, sizeof(data));
        data[5] = 0xFF;
        ECL_JH_RInit(&state, data, 5, 0);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 5));
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_RJump(&state, 3);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 7));
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 4));
        ECL_TEST_ASSERT(state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 1));
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 1));
        ECL_TEST_ASSERT(! state.is_valid);
    }
    {
        memset(data, 0, sizeof(data));
        data[5] = 0xFF;
        ECL_JH_RInit(&state, data, 5, 0);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_RJump(&state, 4);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_RJump(&state, 1);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 8));
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 8));
        ECL_TEST_ASSERT(! state.is_valid);
    }
    {
        memset(data, 0, sizeof(data));
        data[5] = 0xFF;
        ECL_JH_RInit(&state, data, 5, 0);
        ECL_TEST_ASSERT(state.is_valid);
        ECL_JH_RJump(&state, 5);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_JH_RJump(&state, 1);
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 8));
        ECL_TEST_ASSERT(! state.is_valid);
        ECL_TEST_ASSERT(0 == ECL_JH_Read(&state, 8));
        ECL_TEST_ASSERT(! state.is_valid);
    }
}


NTEST(test_string_constants) {
    NTEST_SUPPRESS_UNUSED;
    // test some weird stuff used in other tests for comfortability
    approve(char(3) == "\x3\x5"[0]);
    approve(char(5) == "\x3\x5"[1]);
    approve(char(5) == "\x0\x5"[1]);
    approve(char(0x70) == "\x70\x75"[0]);
    approve(char(0x75) == "\x70\x75"[1]);
}

NTEST(test_ECL_LogSize) {
    NTEST_SUPPRESS_UNUSED;
    ECL_TEST_COMPARE(ECL_LogRoundUp(0), 1);
    ECL_TEST_COMPARE(ECL_LogRoundUp(1), 1);
    ECL_TEST_COMPARE(ECL_LogRoundUp(2), 1);
    ECL_TEST_COMPARE(ECL_LogRoundUp(3), 2);
    ECL_TEST_COMPARE(ECL_LogRoundUp(4), 2);
    ECL_TEST_COMPARE(ECL_LogRoundUp(5), 3);
    ECL_TEST_COMPARE(ECL_LogRoundUp(8), 3);
    ECL_TEST_COMPARE(ECL_LogRoundUp(9), 4);
    ECL_TEST_COMPARE(ECL_LogRoundUp(16), 4);
    ECL_TEST_COMPARE(ECL_LogRoundUp(17), 5);
    ECL_TEST_COMPARE(ECL_LogRoundUp(32), 5);
    ECL_TEST_COMPARE(ECL_LogRoundUp(33), 6);
    ECL_TEST_COMPARE(ECL_LogRoundUp(64), 6);
    ECL_TEST_COMPARE(ECL_LogRoundUp(65), 7);
    ECL_TEST_COMPARE(ECL_LogRoundUp(128), 7);
    ECL_TEST_COMPARE(ECL_LogRoundUp(129), 8);
}

NTEST(test_ECL_GetAlignedPointer2) {
    NTEST_SUPPRESS_UNUSED;
    typedef uint16_t Ty;
    const int type_size = sizeof(Ty);
    const int buf_size = 20;
    uint8_t tmp[buf_size];
    const int shift = ((int)tmp) & 1;
    auto ptr = tmp + shift;
    const auto first = (Ty*)(ptr);
    const auto next = (Ty*)(ptr + type_size);
    ECL_TEST_COMPARE(ECL_GetAlignedPointer2(ptr), first);
    ECL_TEST_COMPARE(ECL_GetAlignedPointer2(ptr + 1), next);
    ECL_TEST_COMPARE(ECL_GetAlignedPointer2(ptr + 2), next);
    for(int i = 0; i < (buf_size - type_size); ++i) {
        auto p = ECL_GetAlignedPointer2(tmp + i);
        approve((int(p) & 1) == 0);
    }
}

NTEST(test_ECL_GetAlignedPointerS) {
    NTEST_SUPPRESS_UNUSED;
    typedef ECL_usize Ty;
    const int type_size = sizeof(Ty);
    const int buf_size = 20;
    uint8_t tmp[buf_size];
    const int shift = sizeof(Ty) - (((int)tmp) & (type_size - 1));
    auto ptr = tmp + shift;
    const auto first = (Ty*)(ptr);
    const auto next = (Ty*)(ptr + type_size);
    ECL_TEST_COMPARE(ECL_GetAlignedPointerS(ptr), first);
    ECL_TEST_COMPARE(ECL_GetAlignedPointerS(ptr + 1), next);
    ECL_TEST_COMPARE(ECL_GetAlignedPointerS(ptr + 2), next);
    if(sizeof(Ty) > 2) {
        ECL_TEST_COMPARE(ECL_GetAlignedPointerS(ptr + 3), next);
        ECL_TEST_COMPARE(ECL_GetAlignedPointerS(ptr + 4), next);
        if(sizeof(Ty) > 4) {
            ECL_TEST_COMPARE(ECL_GetAlignedPointerS(ptr + 5), next);
            ECL_TEST_COMPARE(ECL_GetAlignedPointerS(ptr + 6), next);
            ECL_TEST_COMPARE(ECL_GetAlignedPointerS(ptr + 7), next);
            ECL_TEST_COMPARE(ECL_GetAlignedPointerS(ptr + 8), next);
        }
    }
    for(int i = 0; i < (buf_size - type_size); ++i) {
        auto p = ECL_GetAlignedPointerS(tmp + i);
        approve((int(p) & (type_size - 1)) == 0);
    }
}
