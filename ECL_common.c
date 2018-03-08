/*
 * Copyright 2017 - 2018 Evgeniy Evstratov
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "ECL_JH_States.h"
#include "ECL_utils.h"

static uint8_t ECL_JH_dummy_buffer; /* dummy data storage to simplify some checks */
static const uint8_t c_bmasks8[] = {0, 1, 3, 7, 15, 31, 63, 127, 255};

// utils part
uint8_t ECL_LogRoundUp(ECL_usize value) {
    ECL_usize tmp;
    uint8_t result;
    if(value < 2) {
        return 1;
    }
    result = sizeof(value) * 8;
    tmp = 1;
    tmp <<= result - 1;
    while(value <= tmp) {
        tmp >>= 1;
        --result;
    }
    return result;
}

uint16_t* ECL_GetAlignedPointer2(uint8_t* ptr) {
    ECL_ASSERT(ptr);
    return (uint16_t*) ( (((int)ptr) & 1) ? (ptr + 1) : ptr);
}

ECL_usize* ECL_GetAlignedPointerS(uint8_t* ptr) {
    const int offset = ((int)ptr) & (sizeof(ECL_usize) - 1);
    ECL_ASSERT(ptr);
    return (ECL_usize*)(offset ? (ptr + sizeof(ECL_usize) - offset) : ptr);
}


// JH part
void ECL_JH_WInit(ECL_JH_WState* state, uint8_t* ptr, ECL_usize size, ECL_usize start) {
    uint8_t input_is_valid = 1;
    if((! ptr) || (! size)) {
        input_is_valid = 0;
        ptr = &ECL_JH_dummy_buffer;
        size = 1;
    }
    state->is_valid = input_is_valid * (start < size ? 1 : 0);
    state->end = ptr + size;
    if(state->is_valid) {
        state->byte = ptr + start;
        state->next = state->byte + 1;
    } else {
        state->byte = ptr;
        state->next = state->end; /* if invalid - next==end */
    }
    state->n_bits = 8;
    *(state->byte) = 0;
}

void ECL_JH_RInit(ECL_JH_RState* state, const uint8_t* ptr, ECL_usize size, ECL_usize start) {
    uint8_t input_is_valid = 1;
    if((! ptr) || (! size)) {
        input_is_valid = 0;
        ptr = &ECL_JH_dummy_buffer;
        size = 1;
    }
    state->is_valid = input_is_valid * (start < size ? 1 : 0);
    state->end = ptr + size;
    if(state->is_valid) {
        state->byte = ptr + start;
        state->next = state->byte + 1;
    } else {
        state->byte = ptr;
        state->next = state->end;
    }
    state->n_bits = 8;
}

void ECL_JH_Write(ECL_JH_WState* state, uint8_t value, uint8_t bits) {
    ECL_ASSERT(bits && (bits < 9));
    value &= c_bmasks8[bits];
    if(bits <= state->n_bits) { /* fits easily */
        *(state->byte) |= value << (8 - state->n_bits);
        state->n_bits -= bits;
    } else if(! state->n_bits) { /* to next */
        if(state->next == state->end) {
            state->is_valid = 0;
            return;
        }
        state->byte = state->next;
        ++(state->next);
        *(state->byte) = value;
        state->n_bits = 8 - bits;
    } else { /* 2 parts */
        *(state->byte) |= value << (8 - state->n_bits);
        if(state->next == state->end) {
            state->is_valid = 0;
            return;
        }
        state->byte = state->next;
        ++(state->next);
        *(state->byte) = value >> state->n_bits;
        state->n_bits = 8 - bits + state->n_bits;
    }
}

uint8_t ECL_JH_Read(ECL_JH_RState* state, uint8_t bits) {
    uint8_t res;
    ECL_ASSERT(bits && (bits < 9));
    if(bits <= state->n_bits) { /* fits easily */
        res = *(state->byte) >> (8 - state->n_bits);
        state->n_bits -= bits;
    } else if(! state->n_bits) { /* to next */
        if(state->next == state->end) {
            state->is_valid = 0;
            return 0;
        }
        state->byte = state->next;
        ++(state->next);
        res = *(state->byte);
        state->n_bits = 8 - bits;
    } else { /* 2 parts */
        res = *(state->byte) >> (8 - state->n_bits);
        if(state->next == state->end) {
            state->is_valid = 0;
            return 0;
        }
        state->byte = state->next;
        ++(state->next);
        res |= *(state->byte) << state->n_bits;
        state->n_bits = 8 - bits + state->n_bits;
    }
    return res & c_bmasks8[bits];
}

void ECL_JH_WJump(ECL_JH_WState* state, ECL_usize distance) {
    if((state->next + distance) <= state->end) {
        state->next += distance;
    } else {
        state->next = state->end;
        state->is_valid = 0;
    }
}

void ECL_JH_RJump(ECL_JH_RState* state, ECL_usize distance) {
    if((state->next + distance) <= state->end) {
        state->next += distance;
    } else {
        state->next = state->end;
        state->is_valid = 0;
    }
}


#ifdef ECL_USE_BRANCHLESS
    #define ECL_CALC_E(value, n_bits) \
        (((ECL_usize)((1 << (n_bits)) - 1) - (value)) >> (ECL_SIZE_TYPE_BITS_COUNT - 1 - (n_bits))) & (1 << (n_bits));

#else
    #define ECL_CALC_E(value, n_bits) \
        ((value) > ((ECL_usize)(1 << (n_bits)) - 1) ? (1 << (n_bits)) : 0)

#endif

/*
    this should have been done in C++ templates, but unfortunately embedded rather use C.
    0 < num < 8.
*/
#define ECL_E_NUMBER_DEFINE_SIMPLE_IMPL(num) \
    void ECL_JH_Write_E##num(ECL_JH_WState* state, ECL_usize value) {          \
        do {                                                                   \
            const uint8_t e = ECL_CALC_E(value, num);                          \
            ECL_JH_Write(state, (uint8_t)value | e, num + 1);                  \
            value >>= num;                                                     \
        } while(value);                                                        \
    }                                                                          \
    ECL_usize ECL_JH_Read_E##num(ECL_JH_RState* state) {                       \
        ECL_usize result;                                                      \
        uint8_t shift;                                                         \
        shift = 0;                                                             \
        result = 0;                                                            \
        do {                                                                   \
            const uint8_t code = ECL_JH_Read(state, (num + 1));                \
            result |= ((ECL_usize)(code & ((1 << num) - 1))) << shift;         \
            if(! (code & (1 << num))) {                                        \
                break;                                                         \
            }                                                                  \
            shift += num;                                                      \
        } while(shift < ECL_SIZE_TYPE_BITS_COUNT);                             \
        return result;                                                         \
    }                                                                          \
    uint8_t ECL_Evaluate_E##num(ECL_usize number) {                            \
        uint8_t result = 0;                                                    \
        do {                                                                   \
            number >>= num;                                                    \
            result += num + 1;                                                 \
        } while(number);                                                       \
        return result;                                                         \
    }

ECL_E_NUMBER_DEFINE_SIMPLE_IMPL(2)
ECL_E_NUMBER_DEFINE_SIMPLE_IMPL(3)
ECL_E_NUMBER_DEFINE_SIMPLE_IMPL(4)
ECL_E_NUMBER_DEFINE_SIMPLE_IMPL(5)
ECL_E_NUMBER_DEFINE_SIMPLE_IMPL(6)
ECL_E_NUMBER_DEFINE_SIMPLE_IMPL(7)

#undef ECL_E_NUMBER_DEFINE_SIMPLE_IMPL

/*
    0 < num1 < 8.
    0 < num2 < 8.
*/
#define ECL_E_NUMBER_DEFINE_X2_IMPL(num1, num2)                                 \
    void ECL_JH_Write_E##num1##E##num2(ECL_JH_WState* state, ECL_usize value) { \
        uint8_t e = ECL_CALC_E(value, num1);                                    \
        ECL_JH_Write(state, (uint8_t)value | e, num1 + 1);                      \
        value >>= num1;                                                         \
        while(value) {                                                          \
            e = ECL_CALC_E(value, num2);                                        \
            ECL_JH_Write(state, (uint8_t)value | e, num2 + 1);                  \
            value >>= num2;                                                     \
        }                                                                       \
    }                                                                           \
    ECL_usize ECL_JH_Read_E##num1##E##num2(ECL_JH_RState* state) {              \
        ECL_usize result;                                                       \
        uint8_t code, shift;                                                    \
        code = ECL_JH_Read(state, num1 + 1);                                    \
        result = code & ((1 << num1) - 1);                                      \
        if(code & (1 << num1)) {                                                \
            shift = num1;                                                       \
            do {                                                                \
                code = ECL_JH_Read(state, num2 + 1);                            \
                result |= ((ECL_usize)(code & ((1 << num2) - 1))) << shift;     \
                if(! (code & (1 << num2))) {                                    \
                    break;                                                      \
                }                                                               \
                shift += num2;                                                  \
            } while(shift < ECL_SIZE_TYPE_BITS_COUNT);                          \
        }                                                                       \
        return result;                                                          \
    }                                                                           \
    uint8_t ECL_Evaluate_E##num1##E##num2(ECL_usize number) {                   \
        uint8_t result = num1 + 1;                                              \
        number >>= num1;                                                        \
        while(number) {                                                         \
            number >>= num2;                                                    \
            result += num2 + 1;                                                 \
        }                                                                       \
        return result;                                                          \
    }

ECL_E_NUMBER_DEFINE_X2_IMPL(4, 5)
ECL_E_NUMBER_DEFINE_X2_IMPL(5, 2)
ECL_E_NUMBER_DEFINE_X2_IMPL(5, 3)
ECL_E_NUMBER_DEFINE_X2_IMPL(5, 4)
ECL_E_NUMBER_DEFINE_X2_IMPL(6, 2)
ECL_E_NUMBER_DEFINE_X2_IMPL(6, 3)
ECL_E_NUMBER_DEFINE_X2_IMPL(6, 4)
ECL_E_NUMBER_DEFINE_X2_IMPL(7, 2)
ECL_E_NUMBER_DEFINE_X2_IMPL(7, 3)
ECL_E_NUMBER_DEFINE_X2_IMPL(7, 4)

#undef ECL_E_NUMBER_DEFINE_X2_IMPL

/* cleanup in case of compiling as single file */
#undef ECL_CALC_E
#undef ECL_CHECK_E_AND_SHIFT
