#include "../ECL_JH_States.h"
#include "ntest/ntest.h"

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
        uint64_t value;                                                                     \
        ECL_JH_WState wstate;                                                               \
        ECL_JH_RState rstate;                                                               \
        uint8_t* data;                                                                      \
        ECL_usize retrieved;                                                                \
        ECL_usize n_bytes;                                                                  \
        ECL_usize n_bits = 0;                                                               \
        for(value = 0; value < limit; value = ECL_TEST_E_NEXT_VALUE(value)) {               \
            n_bits += eval_func(value);                                                     \
        }                                                                                   \
        n_bytes = (n_bits / 8) + 1;                                                         \
        data = (uint8_t*)malloc(n_bytes);                                                   \
        ECL_TEST_ASSERT(data);                                                              \
        ECL_JH_WInit(&wstate, data, n_bytes, 0);                                            \
        for(value = 0; value < limit; value = ECL_TEST_E_NEXT_VALUE(value)) {               \
            write_func(&wstate, value);                                                     \
        }                                                                                   \
        ECL_TEST_ASSERT(wstate.is_valid);                                                   \
        ECL_JH_RInit(&rstate, data, n_bytes, 0);                                            \
        for(value = 0; value < limit; value = ECL_TEST_E_NEXT_VALUE(value)) {               \
            retrieved = read_func(&rstate);                                                 \
            ECL_TEST_COMPARE((ECL_usize)value, retrieved);                                  \
        }                                                                                   \
        ECL_TEST_ASSERT(rstate.is_valid);                                                   \
        ECL_TEST_ASSERT(wstate.byte == rstate.byte);                                        \
        ECL_TEST_ASSERT(wstate.next == rstate.next);                                        \
        ECL_TEST_ASSERT(wstate.n_bits == rstate.n_bits);                                    \
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
