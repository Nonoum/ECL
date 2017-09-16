#include "ECL_NanoLZ.h"
#include "ECL_JH_States.h"

#include <stdbool.h>

typedef struct {
    ECL_JH_WState stream;
    ECL_usize n_new; // amount of new raw values
    ECL_ssize offset; // offset / distance
    ECL_usize n_copy; // amount of repeated values to copy
} ECL_NanoLZ_CompressorState;

typedef struct {
    ECL_JH_RState stream;
    ECL_usize n_new;
    ECL_ssize offset;
    ECL_usize n_copy;
} ECL_NanoLZ_DecompressorState;

/*
    ECL_NanoLZ_SchemeCoder is a pointer to coder function.
    ECL_NanoLZ_SchemeDecoder is a pointer to decoder function.
*/
typedef bool(*ECL_NanoLZ_SchemeCoder)(ECL_NanoLZ_CompressorState*);
typedef void(*ECL_NanoLZ_SchemeDecoder)(ECL_NanoLZ_DecompressorState*);

#include "ECL_NanoLZ_schemes.c.inl"

ECL_usize ECL_NanoLZ_CalcEqualLength(const uint8_t* src1, const uint8_t* src2, ECL_usize limit) {
    // TODO dummy version, optimize later
    ECL_ssize i;
    for(i = 0; i < limit; ++i) {
        if(src1[i] != src2[i]) {
            return i;
        }
    }
    return limit;
}

ECL_usize ECL_NanoLZ_Compress_slow(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit) {
    ECL_NanoLZ_CompressorState state;
    const uint8_t* src_start;
    const uint8_t* src_end;
    const uint8_t* search_end;
    const uint8_t* first_undone;
    const uint8_t* back_search_end;
    const uint8_t* candidate;
    uint8_t* tmp;
    ECL_NanoLZ_SchemeCoder coder;
    ECL_usize limit_length, curr_length;
    uint16_t w1;

    coder = ECL_NanoLZ_GetSchemeCoder(scheme);
    if(! coder) {
        return 0;
    }
    ECL_JH_WInit(&state.stream, dst, dst_size, 1);
    if((! src) || (! state.stream.is_valid)) {
        return 0;
    }
    *dst = *src; // copy first byte as is
    src_start = src;
    src_end = src + src_size;
    search_end = src_end - 1;
    first_undone = src + 1;
    for(src = first_undone; src < search_end;) {
        state.n_new = src - first_undone;
        state.n_copy = 0;
        back_search_end = src - ((src - src_start) < search_limit ? (src - src_start) : search_limit);
        candidate = src;
        w1 = *(uint16_t*)candidate;
        limit_length = src_end - src - 2;
        for(--candidate; candidate >= back_search_end; --candidate) {
            if(w1 != *((uint16_t*)candidate)) {
                continue;
            }
            curr_length = ECL_NanoLZ_CalcEqualLength(src + 2, candidate + 2, limit_length) + 2;
            if(curr_length > state.n_copy) {
                state.n_copy = curr_length;
                state.offset = src - candidate;
                if(state.n_copy == (limit_length + 2)) {
                    break;
                }
            }
        }
        if((state.n_copy > 1) && (*coder)(&state)) {
            tmp = state.stream.next;
            ECL_JH_WJump(&state.stream, state.n_new);
            if(state.stream.is_valid) {
                if(state.n_new) {
                    memcpy(tmp, first_undone, state.n_new);
                }
                first_undone += state.n_new + state.n_copy;
                src = first_undone;
            } else {
                break;
            }
        } else {
            ++src;
        }
    }
    // main cycle done. dump last seq
    state.n_new = src_end - first_undone;
    if(state.stream.is_valid && state.n_new) {
        ECL_ASSERT(src == search_end);
        state.n_copy = 0;
        (*coder)(&state);
        tmp = state.stream.next;
        ECL_JH_WJump(&state.stream, state.n_new);
        if(state.stream.is_valid) {
            memcpy(tmp, first_undone, state.n_new);
        }
    }
    if(! state.stream.is_valid) {
        return 0;
    }
    return state.stream.next - dst;
}

// -----------------------------------------------------------------------

ECL_usize ECL_NanoLZ_Decompress(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size) {
    ECL_NanoLZ_DecompressorState state;
    ECL_NanoLZ_SchemeDecoder decoder;
    ECL_usize dst_pos;

    decoder = ECL_NanoLZ_GetSchemeDecoder(scheme);
    if(! decoder) {
        return 0;
    }
    ECL_JH_RInit(&state.stream, src, src_size, 1);
    if((! dst) || (! state.stream.is_valid) || (! dst_size)) {
        return 0;
    }
    *dst = *src; // copy first byte as is
    for(dst_pos = 1; dst_pos < dst_size;) {
        (*decoder)(&state);
        if(! state.stream.is_valid) {
            break; // error in stream
        }
        if((state.n_new + state.n_copy) > (dst_size - dst_pos)) {
            break; // output doesn't fit this
        }
        if(state.n_new) {
            const uint8_t* src_block_start = state.stream.next;
            ECL_JH_RJump(&state.stream, state.n_new);
            if(! state.stream.is_valid) {
                break; // don't have enough data in stream
            }
            memcpy(dst + dst_pos, src_block_start, state.n_new);
            dst_pos += state.n_new;
        }
        if(state.n_copy) {
            if(state.offset > dst_pos) {
                break; // points outside of data. error in stream
            }
            if(state.offset >= state.n_copy) { // regular block
                memcpy(dst + dst_pos, dst + dst_pos - state.offset, state.n_copy);
            } else { // interleaved (cycled) block
                if(state.offset == 1) {
                    memset(dst + dst_pos, dst[dst_pos - 1], state.n_copy);
                } else {
                    uint8_t* ptr_end;
                    uint8_t* ptr = dst + dst_pos;
                    for(ptr_end = ptr + state.n_copy; ptr < ptr_end; ++ptr) {
                        *ptr = *(ptr - state.offset);
                    }
                }
            }
        }
        dst_pos += state.n_copy;
    }
    return dst_pos;
}
