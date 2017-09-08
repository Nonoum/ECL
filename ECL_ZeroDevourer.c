#include "ECL_ZeroDevourer.h"
#include "ECL_JH_States.h"

#define ECL_AS(pointer, type) *((type*)(pointer))
#define ECL_COPY(dst, src, offset, type) ECL_AS(dst + offset, type) = ECL_AS(src + offset, type)

#if 0
#define ECL_MEMCPY(dst, src, size) \
    { \
        if((size) < 9) { \
            switch (size) { \
            case 0: break; \
            case 1: ECL_COPY(dst, src, 0, uint8_t); break; \
            case 2: ECL_COPY(dst, src, 0, uint16_t); break; \
            case 3: ECL_COPY(dst, src, 0, uint16_t); ECL_COPY(dst, src, 2, uint8_t); break; \
            case 4: ECL_COPY(dst, src, 0, uint32_t); break; \
            case 5: ECL_COPY(dst, src, 0, uint32_t); ECL_COPY(dst, src, 4, uint8_t); break; \
            case 6: ECL_COPY(dst, src, 0, uint32_t); ECL_COPY(dst, src, 4, uint16_t); break; \
            case 7: ECL_COPY(dst, src, 0, uint32_t); ECL_COPY(dst, src, 4, uint16_t); ECL_COPY(dst, src, 6, uint8_t); break; \
            case 8: ECL_COPY(dst, src, 0, uint32_t); ECL_COPY(dst, src, 4, uint32_t); break; \
            } \
        } else { \
            memcpy(dst, src, size); \
        } \
    }
#elif 0
#define ECL_MEMCPY(dst, src, size) \
    { \
        if((size) < 5) { \
            ECL_usize i; \
            for(i = 0; i < (size); ++i) { \
                (dst)[i] = (src)[i]; \
            } \
        } else { \
            memcpy(dst, src, size); \
        } \
    }
#elif 0
#define ECL_MEMCPY(dst, src, size) \
    { \
        ECL_usize i; \
        for(i = 0; i < (size-4); i += 4) { \
            ECL_AS(dst + i, uint32_t) = ECL_AS(src + i, uint32_t); \
        } \
        for(; i < (size); ++i) { \
            (dst)[i] = (src)[i]; \
        } \
    }
#else
#define ECL_MEMCPY memcpy
#endif

static void ECL_ZeroDevourer_DumpSeq100(ECL_JH_WState* state, const uint8_t* src, ECL_usize cnt_x) {
    ECL_ASSERT((cnt_x >= 1) && (cnt_x <= 4));
    uint8_t* dst;
    ECL_JH_Write(state, 0x01, 3);
    ECL_JH_Write(state, cnt_x - 1, 2);
    dst = state->next;
    ECL_JH_WJump(state, cnt_x);
    if(state->is_valid) {
        ECL_MEMCPY(dst, src, cnt_x);
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
        ECL_MEMCPY(dst, src, cnt_x);
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
        ECL_MEMCPY(dst, src, cnt_x);
    }
}

static void ECL_ZeroDevourer_ReadSeq(ECL_JH_RState* state, ECL_usize* cnt_x, ECL_usize* cnt_0) {
    if(! ECL_JH_Read(state, 1)) {
        *cnt_x = 0;
        *cnt_0 = 1;
        return;
    }
    switch (ECL_JH_Read(state, 2)) {
    case 0: // seq 1.00
        *cnt_x = ECL_JH_Read(state, 2) + 1;
        *cnt_0 = 1;
        break;
    case 1: // seq 1.01
        *cnt_x = ECL_JH_Read(state, 4) + 5;
        *cnt_0 = 1;
        break;
    case 2: // seq 1.10
        *cnt_x = 0;
        *cnt_0 = ECL_JH_Read_E4(state) + 9;
        break;
    case 3: // seq 1.11
        *cnt_x = ECL_JH_Read_E6E3(state) + 1;
        *cnt_0 = 0;
        break;
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
    ECL_usize dst_pos, cnt_x, cnt_0;
    const uint8_t* src_block_start;

    ECL_JH_RInit(&state, src, src_size, 0);
    if((! dst) || (! state.is_valid)) {
        return 0;
    }
    for(dst_pos = 0; dst_pos < dst_size;) {
        ECL_ZeroDevourer_ReadSeq(&state, &cnt_x, &cnt_0);
        if(state.is_valid && ((dst_pos + cnt_x + cnt_0) <= dst_size)) {
            if(cnt_x) {
                src_block_start = state.next;
                ECL_JH_RJump(&state, cnt_x);
                if(! state.is_valid) {
                    break;
                }
                ECL_MEMCPY(dst + dst_pos, src_block_start, cnt_x);
                dst_pos += cnt_x;
            }
            if(cnt_0) {
                memset(dst + dst_pos, 0, cnt_0);
                dst_pos += cnt_0;
            }
        } else {
            break;
        }
    }
    return dst_pos;
}
