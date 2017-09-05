#include "ECL_JH_States.h"

static uint8_t ECL_JH_dummy_buffer; // dummy data storage to simplify some checks
static const uint8_t c_bmasks8[] = {0, 1, 3, 7, 15, 31, 63, 127, 255};
static const uint8_t c_rbmasks8[] = {255, 127, 63, 31, 15, 7, 3, 1, 0};

void ECL_JH_WInit(ECL_JH_WState* state, uint8_t* ptr, ECL_usize size, ECL_usize start) {
    uint8_t input_is_valid = 1;
    if((! ptr) || (! size)) {
        input_is_valid = 0;
        ptr = &ECL_JH_dummy_buffer;
        size = 1;
    }
    state->is_valid = input_is_valid * (start < size ? 1 : 0);
    state->byte = ptr + start * state->is_valid;
    state->next = state->byte + state->is_valid;
    state->end = ptr + size;
    state->n_bits = 8;
    *ptr = 0;
}

void ECL_JH_RInit(ECL_JH_RState* state, const uint8_t* ptr, ECL_usize size, ECL_usize start) {
    uint8_t input_is_valid = 1;
    if((! ptr) || (! size)) {
        input_is_valid = 0;
        ptr = &ECL_JH_dummy_buffer;
        size = 1;
    }
    state->is_valid = input_is_valid * (start < size ? 1 : 0);
    state->byte = ptr + start * state->is_valid;
    state->next = state->byte + state->is_valid;
    state->end = ptr + size;
    state->n_bits = 8;
}

void ECL_JH_Write(ECL_JH_WState* state, uint8_t value, uint8_t bits) {
    value &= c_bmasks8[bits];
    if(bits <= state->n_bits) { // fits easily
        *(state->byte) |= value << (8 - state->n_bits);
        state->n_bits -= bits;
    } else if(! state->n_bits) { // to next
        if(state->next == state->end) {
            state->is_valid = 0;
            return;
        }
        state->byte = state->next;
        state->next += state->is_valid;
        *(state->byte) = value;
        state->n_bits = 8 - bits;
    } else { // 2 parts
        *(state->byte) |= value << (8 - state->n_bits);
        value >>= state->n_bits;
        bits -= state->n_bits;
        state->n_bits = 0;
        ECL_JH_Write(state, value, bits); // TODO optimize all of it
    }
}

uint8_t ECL_JH_Read(ECL_JH_RState* state, uint8_t bits) {
    uint8_t b, res;
    if(bits <= state->n_bits) { // fits easily
        res = *(state->byte) >> (8 - state->n_bits);
        state->n_bits -= bits;
    } else if(! state->n_bits) { // to next
        if(state->next == state->end) {
            state->is_valid = 0;
            return 0;
        }
        state->byte = state->next;
        state->next += state->is_valid;
        res = *(state->byte);
        state->n_bits = 8 - bits;
    } else { // 2 parts
        b = state->n_bits;
        res = (*(state->byte) >> (8 - b)) & c_bmasks8[b];
        state->n_bits = 0;
        res |= ECL_JH_Read(state, bits - b) << b;
    }
    res &= c_bmasks8[bits];
    return res;
}

void ECL_JH_WJump(ECL_JH_WState* state, ECL_usize distance) {
    if((state->next + distance) <= state->end) {
        state->next += distance;
    } else {
        state->is_valid = 0;
    }
}

void ECL_JH_RJump(ECL_JH_RState* state, ECL_usize distance) {
    if((state->next + distance) <= state->end) {
        state->next += distance;
    } else {
        state->is_valid = 0;
    }
}

#ifdef ECL_USE_BRANCHLESS
#define ECL_CALC_E(value, n_bits) \
    ((ECL_usize((1 << (n_bits)) - 1) - (value)) >> (ECL_SIZE_TYPE_BITS_COUNT - 1 - (n_bits))) & (1 << (n_bits));

#define ECL_CHECK_E_AND_SHIFT(the_e, the_shift, n_bits) \
    ((the_e) & (uint8_t((the_shift) - ECL_SIZE_TYPE_BITS_COUNT - 1) >> (7 - (n_bits))))

#else
#define ECL_CALC_E(value, n_bits) \
    ((value) > ECL_usize((1 << (n_bits)) - 1) ? (1 << (n_bits)) : 0)

#define ECL_CHECK_E_AND_SHIFT(the_e, the_shift, n_bits) \
    ((e) && ((shift) < ECL_SIZE_TYPE_BITS_COUNT))

#endif

void ECL_JH_Write_E4(ECL_JH_WState* state, ECL_usize value) {
    uint8_t e; // extension flag
    do {
        e = ECL_CALC_E(value, 4);
        ECL_JH_Write(state, (value & 0x0F) | e, 5);
        value >>= 4;
    } while(value);
}

void ECL_JH_Write_E7E4(ECL_JH_WState* state, ECL_usize value) {
    uint8_t e; // extension flag
    e = ECL_CALC_E(value, 7);
    ECL_JH_Write(state, (value & 0x7F) | e, 8);
    value >>= 7;
    while(value) {
        e = ECL_CALC_E(value, 4);
        ECL_JH_Write(state, (value & 0x0F) | e, 5);
        value >>= 4;
    }
}

void ECL_JH_Write_E6E3(ECL_JH_WState* state, ECL_usize value) {
    uint8_t e; // extension flag
    e = ECL_CALC_E(value, 6);
    ECL_JH_Write(state, (value & 0x3F) | e, 7);
    value >>= 6;
    while(value) {
        e = ECL_CALC_E(value, 3);
        ECL_JH_Write(state, (value & 0x07) | e, 4);
        value >>= 3;
    }
}

ECL_usize ECL_JH_Read_E4(ECL_JH_RState* state) {
    ECL_usize result;
    uint8_t e, code, shift;
    shift = 0;
    result = 0;
    do {
        code = ECL_JH_Read(state, 5);
        e = code & 0x10;
        result |= (code & 0x0F) << shift;
        shift += 4;
    } while(ECL_CHECK_E_AND_SHIFT(e, shift, 4));
    // TODO set up (multiply) is_valid if overflowed?
    return result;
}

ECL_usize ECL_JH_Read_E7E4(ECL_JH_RState* state) {
    ECL_usize result;
    uint8_t e, code, shift;
    code = ECL_JH_Read(state, 8);
    result = code & 0x7F;
    e = code & 0x80;
    shift = 7;
    if(e) {
        do {
            code = ECL_JH_Read(state, 5);
            e = code & 0x10;
            result |= (code & 0x0F) << shift;
            shift += 4;
        } while(ECL_CHECK_E_AND_SHIFT(e, shift, 4));
    }
    return result;
}

ECL_usize ECL_JH_Read_E6E3(ECL_JH_RState* state) {
    ECL_usize result;
    uint8_t e, code, shift;
    code = ECL_JH_Read(state, 7);
    result = code & 0x3F;
    e = code & 0x40;
    shift = 6;
    if(e) {
        do {
            code = ECL_JH_Read(state, 4);
            e = code & 0x8;
            result |= (code & 0x07) << shift;
            shift += 3;
        } while(ECL_CHECK_E_AND_SHIFT(e, shift, 3));
    }
    return result;
}

uint8_t ECL_Evaluate_E4(ECL_usize number) {
    uint8_t result = 0;
    do {
        number >>= 4;
        result += 5;
    } while(number);
    return result;
}

uint8_t ECL_Evaluate_E7E4(ECL_usize number) {
    uint8_t result = 8;
    number >>= 7;
    while(number) {
        number >>= 4;
        result += 5;
    }
    return result;
}

uint8_t ECL_Evaluate_E6E3(ECL_usize number) {
    uint8_t result = 7;
    number >>= 6;
    while(number) {
        number >>= 3;
        result += 4;
    }
    return result;
}
