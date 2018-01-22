#include "ECL_ZeroDevourer.h"
#include "ECL_JH_States.h"

#include <stdbool.h>

static void ECL_ZeroDevourer_DumpSeq100(ECL_JH_WState* state, const uint8_t* src, ECL_usize cnt_x) {
    ECL_ASSERT((cnt_x >= 1) && (cnt_x <= 4));
    uint8_t* dst;
    ECL_JH_Write(state, 0x01, 3);
    ECL_JH_Write(state, cnt_x - 1, 2);
    dst = state->next;
    ECL_JH_WJump(state, cnt_x);
    if(state->is_valid) {
        ECL_usize i;
        for(i = 0; i < cnt_x; ++i) {
            dst[i] = src[i];
        }
    }
}

static void ECL_ZeroDevourer_DumpSeq101(ECL_JH_WState* state, const uint8_t* src, ECL_usize cnt_x) {
    ECL_ASSERT((cnt_x >= 5) && (cnt_x <= 20));
    uint8_t* dst;
    ECL_JH_Write(state, 0x03, 3);
    ECL_JH_Write(state, cnt_x - 5, 4);
    dst = state->next;
    ECL_JH_WJump(state, cnt_x);
    if(state->is_valid) {
        memcpy(dst, src, cnt_x);
    }
}

static void ECL_ZeroDevourer_DumpSeq110(ECL_JH_WState* state, ECL_usize cnt_0) {
    ECL_ASSERT(cnt_0 >= 9);
    ECL_JH_Write(state, 0x05, 3);
    ECL_JH_Write_E4(state, cnt_0 - 9);
}

static void ECL_ZeroDevourer_DumpSeq111(ECL_JH_WState* state, const uint8_t* src, ECL_usize cnt_x) {
    ECL_ASSERT(cnt_x >= 1);
    uint8_t* dst;
    ECL_JH_Write(state, 0x07, 3);
    ECL_JH_Write_E6E3(state, cnt_x - 1);
    dst = state->next;
    ECL_JH_WJump(state, cnt_x);
    if(state->is_valid) {
        memcpy(dst, src, cnt_x);
    }
}

static void ECL_ZeroDevourer_DumpZeroGeneric(ECL_JH_WState* state, ECL_usize cnt_0) {
    // cnt_0 > 0
    if(cnt_0 >= 9) {
        ECL_ZeroDevourer_DumpSeq110(state, cnt_0);
    } else {
        ECL_JH_Write(state, 0, cnt_0);
    }
}

static void ECL_ZeroDevourer_DumpGeneric(ECL_JH_WState* state, const uint8_t* src, ECL_usize cnt_x, ECL_usize cnt_0) {
    // cnt_x + cnt_0 > 0
    if(! cnt_x) {
        ECL_ZeroDevourer_DumpZeroGeneric(state, cnt_0);
        return;
    }
    if(! cnt_0) {
        ECL_ZeroDevourer_DumpSeq111(state, src, cnt_x);
        return;
    }
    if(cnt_x <= 4) {
        ECL_ZeroDevourer_DumpSeq100(state, src, cnt_x);
        --cnt_0;
    } else if(cnt_x <= 20) {
        ECL_ZeroDevourer_DumpSeq101(state, src, cnt_x);
        --cnt_0;
    } else {
        ECL_ZeroDevourer_DumpSeq111(state, src, cnt_x);
    }
    if(cnt_0) {
        ECL_ZeroDevourer_DumpZeroGeneric(state, cnt_0);
    }
}

static bool ECL_ZeroDevourer_IsWorth(ECL_usize cnt_x, ECL_usize cnt_0) {
    ECL_usize bits_needed;
    if(cnt_x <= 20) {
        return true;
    }
    if(cnt_x <= (1U << 9)) {
        return cnt_0 >= 2;
    }
    if(cnt_x <= (1U << 12)) {
        return cnt_0 >= 3;
    }
    bits_needed = 3 + ECL_Evaluate_E6E3(cnt_x - 1);
    if(cnt_0 < 9) {
        bits_needed += cnt_0;
    } else {
        bits_needed += 8;
    }
    return bits_needed <= (cnt_0 << 3);
}

ECL_usize ECL_ZeroDevourer_Compress(const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size) {
    ECL_JH_WState state;
    ECL_usize cnt_x, cnt_0;
    const uint8_t* first_undone;
    const uint8_t* first_x;
    const uint8_t* first_0;
    const uint8_t* const src_end = src + src_size;

    ECL_JH_WInit(&state, dst, dst_size, 0);
    if((! src) || (! state.is_valid)) {
        return 0;
    }
    first_undone = src;
    first_x = src;
    while((first_undone < src_end) && (state.is_valid)) {
        for(first_0 = first_x; (first_0 < src_end) && *first_0; ++first_0); // search for zero from where last search ended

        cnt_x = first_0 - first_undone; // count of non-zeroes in beginning
        if(first_0 == src_end) { // complete it (1)
            ECL_ZeroDevourer_DumpSeq111(&state, first_undone, cnt_x);
            break;
        }
        // we found zero, find next non-zero
        for(first_x = first_0; (first_x < src_end) && (! *first_x); ++first_x);

        cnt_0 = first_x - first_0; // count of zeroes afterwards
        // stream looks like {first_undone: [xx..x] first_0: [00..0] first_x: ...}
        if(ECL_ZeroDevourer_IsWorth(cnt_x, cnt_0)) {
            ECL_ZeroDevourer_DumpGeneric(&state, first_undone, cnt_x, cnt_0);
            first_undone = first_x;
        }
    }
    if(! state.is_valid) {
        return 0;
    }
    return state.next - dst;
}

ECL_usize ECL_ZeroDevourer_Decompress(const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size) {
    ECL_JH_RState state;
    const uint8_t* dst_start = dst;
    const uint8_t* dst_end = dst + dst_size;
    ECL_JH_RInit(&state, src, src_size, 0);
    if((! dst) || (! state.is_valid)) {
        return 0;
    }
    for(;;) {
        ECL_usize cnt_x, cnt_0;
        const ECL_usize left = dst_end - dst;
        if(! left) {
            break;
        }
        if(! ECL_JH_Read(&state, 1)) {
            if(state.is_valid) {
                *dst = 0;
                ++dst;
            } else {
                return 0;
            }
        } else { // wide codes
            switch (ECL_JH_Read(&state, 2)) {
            case 0: // seq 1.00
                cnt_x = ECL_JH_Read(&state, 2) + 1;
                {
                    const uint8_t* const src_block_start = state.next;
                    ECL_usize i = 0;
                    ECL_JH_RJump(&state, cnt_x);
                    if(state.is_valid && (cnt_x < left)) {
                        dst[cnt_x] = 0;
                        for(; i < cnt_x; ++i) {
                            dst[i] = src_block_start[i];
                        }
                        dst += cnt_x + 1;
                    } else {
                        return 0;
                    }
                }
                break;
            case 1: // seq 1.01
                cnt_x = ECL_JH_Read(&state, 4) + 5;
                {
                    const uint8_t* const src_block_start = state.next;
                    ECL_JH_RJump(&state, cnt_x);
                    if(state.is_valid && (cnt_x < left)) {
                        dst[cnt_x] = 0;
                        memcpy(dst, src_block_start, cnt_x);
                        dst += cnt_x + 1;
                    } else {
                        return 0;
                    }
                }
                break;
            case 2: // seq 1.10
                cnt_0 = ECL_JH_Read_E4(&state) + 9;
                if(state.is_valid && (cnt_0 <= left)) {
                    memset(dst, 0, cnt_0);
                    dst += cnt_0;
                } else {
                    return 0;
                }
                break;
            case 3: // seq 1.11
                cnt_x = ECL_JH_Read_E6E3(&state) + 1;
                {
                    const uint8_t* const src_block_start = state.next;
                    ECL_JH_RJump(&state, cnt_x);
                    if(state.is_valid && (cnt_x <= left)) {
                        memcpy(dst, src_block_start, cnt_x);
                        dst += cnt_x;
                    } else {
                        return 0;
                    }
                }
                break;
            }
        }
    }
    return dst - dst_start;
}
