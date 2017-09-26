#include "ECL_NanoLZ.h"
#include "ECL_JH_States.h"

#include <stdbool.h>

#define ECL_MIN(a, b) ((a) < (b) ? (a) : (b))
#define ECL_MAX(a, b) ((a) > (b) ? (a) : (b))

#define ECL_AS_U16(pointer) *((const uint16_t*)(pointer)) // TODO *READ*
#define ECL_AS_U32(pointer) *((const uint32_t*)(pointer))

#define ECL_CUT_POINTER(pointer, usize_mask) ( ((ECL_usize)(pointer)) & usize_mask )

#define ECL_SET_POINTER(dst_var, src_var, lower_bits_clear, lower_bits_mask) \
    dst_var = (src_var & ~(uint8_t*)lower_bits_mask) | lower_bits_clear; \
    if(src_var < dst_var) dst_var -= 1 + lower_bits_mask;

typedef struct {
    const uint8_t* src_start;
    const uint8_t* src_end;
    const uint8_t* search_end;
    const uint8_t* first_undone;

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

typedef struct {
    ECL_NanoLZ_CompressorState common;
    ECL_usize* buf_map;
    ECL_usize* buf_window;
    ECL_usize window_size;
    ECL_usize window_mask;
} ECL_NanoLZ_CompressorStateEx;

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

static ECL_usize ECL_NanoLZ_CompleteCompression(ECL_NanoLZ_CompressorState* s, ECL_NanoLZ_SchemeCoder coder, const uint8_t* dst_start) {
    uint8_t* tmp;
    s->n_new = s->src_end - s->first_undone;
    if(s->stream.is_valid && s->n_new) {
        s->n_copy = 0;
        (*coder)(s);
        tmp = s->stream.next;
        ECL_JH_WJump(&s->stream, s->n_new);
        if(s->stream.is_valid) {
            memcpy(tmp, s->first_undone, s->n_new);
        }
    }
    if(! s->stream.is_valid) {
        return 0;
    }
    return s->stream.next - dst_start;
}

ECL_usize ECL_NanoLZ_Compress_slow(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit) {
    ECL_NanoLZ_CompressorState state;
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
    state.src_start = src;
    state.src_end = src + src_size;
    state.search_end = state.src_end - 1;
    state.first_undone = src + 1;
    for(src = state.first_undone; src < state.search_end;) {
        state.n_new = src - state.first_undone;
        state.n_copy = 0;
        back_search_end = src - ((src - state.src_start) < search_limit ? (src - state.src_start) : search_limit);
        candidate = src;
        w1 = *(uint16_t*)candidate;
        limit_length = state.src_end - src - 2;
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
                    memcpy(tmp, state.first_undone, state.n_new);
                }
                state.first_undone += state.n_new + state.n_copy;
                src = state.first_undone;
            } else {
                break;
            }
        } else {
            ++src;
        }
    }
    // main cycle done. dump last seq
    return ECL_NanoLZ_CompleteCompression(&state, coder, dst);
}

// 'fast' versions ------------------------------------------------------------------------------

void ECL_NanoLZ_FastParams_Alloc1(ECL_NanoLZ_FastParams* p, uint8_t window_size_bits) {
    p->buf_map = malloc(256 * sizeof(ECL_usize));
    memset(p->buf_map, -1, 256 * sizeof(ECL_usize));

    p->buf_window = malloc((1UL << window_size_bits) * sizeof(ECL_usize));
    p->window_size_bits = window_size_bits;
}

void ECL_NanoLZ_FastParams_Alloc2(ECL_NanoLZ_FastParams* p, uint8_t window_size_bits) {
    p->buf_map = malloc(256 * 256 * sizeof(ECL_usize));
    memset(p->buf_map, -1, 256 * 256 * sizeof(ECL_usize));

    p->buf_window = malloc((1UL << window_size_bits) * sizeof(ECL_usize));
    p->window_size_bits = window_size_bits;
}

void ECL_NanoLZ_FastParams_Destroy(ECL_NanoLZ_FastParams* p) {
    free(p->buf_map);
    free(p->buf_window);
}
/*
static void ECL_NanoLZ_CompressorStateEx_UpdateTables(ECL_NanoLZ_CompressorStateEx* s, const uint8_t* src, ECL_usize n) {
    ECL_usize i, pos;
    uint8_t key;
    pos = src - s->common.src_start;
    for(i = 0; i < n; ++i, ++pos) {
        key = src[i];
        s->buf_window[pos] = s->buf_map[key];
        s->buf_map[key] = pos;
    }
}

ECL_usize ECL_NanoLZ_Compress_fast1(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit, ECL_NanoLZ_FastParams* p) {
    ECL_NanoLZ_CompressorStateEx s;
    ECL_NanoLZ_SchemeCoder coder;
    ECL_usize limit_length, curr_length, checked_idx, n_checks;

    coder = ECL_NanoLZ_GetSchemeCoder(scheme);
    if(! coder) {
        return 0;
    }
    ECL_JH_WInit(&s.common.stream, dst, dst_size, 1);
    if((! src) || (! s.common.stream.is_valid)) {
        return 0;
    }
    ECL_NanoLZ_FastParams_Alloc1(p, 16);
    s.buf_map = (ECL_usize*)p->buf_map;
    s.buf_window = (ECL_usize*)p->buf_window;

    *dst = *src; // copy first byte as is
    s.common.src_start = src;
    s.common.src_end = src + src_size;
    s.common.search_end = s.common.src_end - 1;
    s.common.first_undone = src + 1;
    ECL_NanoLZ_CompressorStateEx_UpdateTables(&s, src, 1);
    for(src = s.common.first_undone; src < s.common.search_end;) {
        const uint8_t* tmp_src1;
        s.common.n_new = src - s.common.first_undone;
        s.common.n_copy = 0;

        limit_length = s.common.src_end - src - 1;
        tmp_src1 = src + 1;
        checked_idx = s.buf_256[*src];
        for(n_checks = 0; (n_checks < search_limit) && (checked_idx != (ECL_usize)-1); ++n_checks) {
            const uint8_t* tmp_src2;
            ECL_ASSERT(checked_idx < src_size);
            tmp_src2 = s.common.src_start + checked_idx + 1;
            curr_length = 0;
            for(; curr_length < limit_length; ++curr_length) {
                if(tmp_src1[curr_length] != tmp_src2[curr_length]) {
                    break;
                }
            }
            curr_length += 1;
            if(curr_length > s.common.n_copy) {
                s.common.n_copy = curr_length;
                s.common.offset = checked_idx;
                if(s.common.n_copy == limit_length) {
                    break;
                }
            }
            checked_idx = s.buf_n[checked_idx];
        }

        s.common.offset = (src - s.common.src_start) - s.common.offset;
        if((s.common.n_copy > 1) && (*coder)(&s.common)) {
            uint8_t* tmp = s.common.stream.next;
            ECL_JH_WJump(&s.common.stream, s.common.n_new);
            if(s.common.stream.is_valid) {
                if(s.common.n_new) {
                    memcpy(tmp, s.common.first_undone, s.common.n_new);
                }
                ECL_NanoLZ_CompressorStateEx_UpdateTables(&s, src, s.common.n_copy);
                s.common.first_undone += s.common.n_new + s.common.n_copy;
                src = s.common.first_undone;
                continue;
            } else {
                break;
            }
        }
        ECL_NanoLZ_CompressorStateEx_UpdateTables(&s, src, 1);
        ++src;
    }
    // main cycle done. dump last seq
    ECL_NanoLZ_FastParams_Destroy(p);
    return ECL_NanoLZ_CompleteCompression(&s.common, coder, dst);
}*/ // TODO

static void ECL_NanoLZ_CompressorStateEx_UpdateTablesW(ECL_NanoLZ_CompressorStateEx* s, const uint8_t* src, ECL_usize n) {
    ECL_usize i, pos;
    uint16_t key;
    pos = src - s->common.src_start;
    for(i = 0; i < n; ++i, ++pos) {
        key = ECL_AS_U16(src + i);
        s->buf_window[pos & s->window_mask] = s->buf_map[key];
        s->buf_map[key] = pos;
    }
}

ECL_usize ECL_NanoLZ_Compress_fast2(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit, ECL_NanoLZ_FastParams* p) {
    ECL_NanoLZ_CompressorStateEx s;
    ECL_NanoLZ_SchemeCoder coder;
    ECL_usize limit_length, curr_length, checked_idx, n_checks;
    ECL_usize pos;
    uint16_t key;

    coder = ECL_NanoLZ_GetSchemeCoder(scheme);
    if(! coder) {
        return 0;
    }
    ECL_JH_WInit(&s.common.stream, dst, dst_size, 1);
    if((! src) || (! s.common.stream.is_valid)) {
        return 0;
    }
    ECL_NanoLZ_FastParams_Alloc2(p, 20); // TODO
    s.buf_map = (ECL_usize*)p->buf_map;
    s.buf_window = (ECL_usize*)p->buf_window;
    s.window_size = 1UL << p->window_size_bits;
    s.window_mask = s.window_size - 1;

    *dst = *src; // copy first byte as is
    s.common.src_start = src;
    s.common.src_end = src + src_size;
    s.common.search_end = s.common.src_end - 1;
    s.common.first_undone = src + 1;
    key = ECL_AS_U16(src);
    s.buf_window[0] = -1;
    s.buf_map[key] = 0;
    for(src = s.common.first_undone; src < s.common.search_end;) {
        const uint8_t* tmp_src1;
        s.common.n_new = src - s.common.first_undone;
        s.common.n_copy = 0;

        pos = src - s.common.src_start;
        limit_length = s.common.src_end - src - 2;
        tmp_src1 = src + 2;
        checked_idx = s.buf_map[ECL_AS_U16(src)];
        for(n_checks = 0; (n_checks < search_limit) && (checked_idx != (ECL_usize)-1); ++n_checks) {
            const uint8_t* tmp_src2;
            ECL_ASSERT(checked_idx < src_size);
            tmp_src2 = s.common.src_start + checked_idx + 2;
            curr_length = 0;
            for(; curr_length < limit_length; ++curr_length) {
                if(tmp_src1[curr_length] != tmp_src2[curr_length]) {
                    break;
                }
            }
            curr_length += 2;
            if(curr_length > s.common.n_copy) {
                s.common.n_copy = curr_length;
                s.common.offset = checked_idx;
                if(s.common.n_copy == limit_length) {
                    break;
                }
            }
            if((pos - checked_idx) > s.window_size) { // ran out of window
                break;
            }
            checked_idx = s.buf_window[checked_idx & s.window_mask];
        }

        s.common.offset = pos - s.common.offset;
        if((s.common.n_copy > 1) && (*coder)(&s.common)) {
            uint8_t* tmp = s.common.stream.next;
            ECL_JH_WJump(&s.common.stream, s.common.n_new);
            if(s.common.stream.is_valid) {
                ECL_usize i;
                if(s.common.n_new) {
                    memcpy(tmp, s.common.first_undone, s.common.n_new);
                }
                for(i = 0; i < s.common.n_copy; ++i, ++pos) {
                    key = ECL_AS_U16(src + i);
                    s.buf_window[pos & s.window_mask] = s.buf_map[key];
                    s.buf_map[key] = pos;
                }
                s.common.first_undone += s.common.n_new + s.common.n_copy;
                src = s.common.first_undone;
                continue;
            } else {
                break;
            }
        }
        key = ECL_AS_U16(src);
        s.buf_window[pos & s.window_mask] = s.buf_map[key];
        s.buf_map[key] = pos;
        ++src;
    }
    // main cycle done. dump last seq
    ECL_NanoLZ_FastParams_Destroy(p);
    return ECL_NanoLZ_CompleteCompression(&s.common, coder, dst);
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

#undef ECL_AS_U16
#undef ECL_AS_U32
