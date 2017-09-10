#include "../ECL_JH_States.h"
#include "ntest/ntest.h"

#include <algorithm>

#define ECL_TEST_E_NEXT_VALUE(value) (((value) * 128) / 127 + 1)

#define ECL_TEST_ASSERT(expr) approve(expr)
#define ECL_TEST_COMPARE(val1, val2)                               \
    {                                                              \
        if((val1) != (val2)) {                                     \
            if(hasntFailed()) {                                    \
                log << "VAL1: " << std::hex << (val1) << " != VAL2: " << (val2) << " "; \
            }                                                      \
            approve(false);                                        \
        } else {                                                   \
            approve(true);                                         \
        }                                                          \
    }

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
        for(int i = 0; i < 1000; ++i) {                                                     \
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

NTEST(test_number_E4) {
    ECL_TEST_E_NUMBER_GENERIC(ECL_JH_Write_E4, ECL_JH_Read_E4, ECL_Evaluate_E4);
}

NTEST(test_number_E7E4) {
    ECL_TEST_E_NUMBER_GENERIC(ECL_JH_Write_E7E4, ECL_JH_Read_E7E4, ECL_Evaluate_E7E4);
}

NTEST(test_number_E6E3) {
    ECL_TEST_E_NUMBER_GENERIC(ECL_JH_Write_E6E3, ECL_JH_Read_E6E3, ECL_Evaluate_E6E3);
}

#undef ECL_TEST_E_NEXT_VALUE

NTEST(test_JH_generic) {
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
