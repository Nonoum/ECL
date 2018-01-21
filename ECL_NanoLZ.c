#include "ECL_NanoLZ.h"
#include "ECL_JH_States.h"

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

// import schemes, define counters if needed
#ifdef ECL_USE_STAT_COUNTERS
    int ECL_NanoLZ_Decompression_OpcodePickCounters[ECL_NANO_LZ_DECOMPRESSION_OPCODE_PICK_COUNTERS_COUNT];
    #define ECL_NANO_LZ_COUNTER_APPEND(index, value) ECL_NanoLZ_Decompression_OpcodePickCounters[index] += value;
    #define ECL_NANO_LZ_COUNTER_CLEARALL() memset(ECL_NanoLZ_Decompression_OpcodePickCounters, 0, sizeof(ECL_NanoLZ_Decompression_OpcodePickCounters));
#else
    #define ECL_NANO_LZ_COUNTER_APPEND(index, value)
    #define ECL_NANO_LZ_COUNTER_CLEARALL()
#endif

#include "ECL_NanoLZ_schemes.c.inl"


static ECL_usize ECL_NanoLZ_CalcEqualLength(const uint8_t* src1, const uint8_t* src2, ECL_usize limit) {
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
    ECL_NanoLZ_SchemeCoder coder = ECL_NanoLZ_GetSchemeCoder(scheme);
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
        const uint8_t* const back_search_end = src - ECL_MIN(src - state.src_start, search_limit);
        const uint8_t* candidate = src;
        const uint16_t w1 = *(uint16_t*)candidate;
        const ECL_usize limit_length = state.src_end - src - 2;
        state.n_new = src - state.first_undone;
        state.n_copy = 0;
        for(--candidate; candidate >= back_search_end; --candidate) {
            ECL_usize curr_length;
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
            uint8_t* tmp = state.stream.next;
            ECL_JH_WJump(&state.stream, state.n_new);
            if(state.stream.is_valid) {
                if(state.n_new) {
                    memcpy(tmp, state.first_undone, state.n_new);
                }
                src += state.n_copy;
                state.first_undone = src;
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

bool ECL_NanoLZ_FastParams_Alloc1(ECL_NanoLZ_FastParams* p, uint8_t window_size_bits) {
    p->buf_map = malloc(ECL_NANO_LZ_GET_FAST1_MAP_BUF_SIZE());
    p->buf_window = malloc(ECL_NANO_LZ_GET_FAST1_WINDOW_BUF_SIZE(window_size_bits));
    p->window_size_bits = window_size_bits;
    return (p->buf_map) && (p->buf_window);
}

bool ECL_NanoLZ_FastParams_Alloc2(ECL_NanoLZ_FastParams* p, uint8_t window_size_bits) {
    p->buf_map = malloc(ECL_NANO_LZ_GET_FAST2_MAP_BUF_SIZE());
    p->buf_window = malloc(ECL_NANO_LZ_GET_FAST2_WINDOW_BUF_SIZE(window_size_bits));
    p->window_size_bits = window_size_bits;
    return (p->buf_map) && (p->buf_window);
}

void ECL_NanoLZ_FastParams_Destroy(ECL_NanoLZ_FastParams* p) {
    free(p->buf_map);
    free(p->buf_window);
}

ECL_usize ECL_NanoLZ_Compress_fast1(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit, ECL_NanoLZ_FastParams* p) {
    ECL_NanoLZ_CompressorState state;
    ECL_NanoLZ_SchemeCoder coder = ECL_NanoLZ_GetSchemeCoder(scheme);
    if((! coder) || (! p)) {
        return 0;
    }
    ECL_JH_WInit(&state.stream, dst, dst_size, 1);
    if((! src) || (! state.stream.is_valid)) {
        return 0;
    }
    memset(p->buf_map, -1, ECL_NANO_LZ_GET_FAST1_MAP_BUF_SIZE());
    {
        ECL_usize* const buf_map = (ECL_usize*)p->buf_map;
        ECL_usize* const buf_window = (ECL_usize*)p->buf_window;
        const ECL_usize window_size = 1UL << p->window_size_bits;
        const ECL_usize window_mask = (1UL << p->window_size_bits) - 1;
        ECL_usize pos;

        *dst = *src; // copy first byte as is
        state.src_start = src;
        state.src_end = src + src_size;
        state.search_end = state.src_end - 1;
        state.first_undone = src + 1;
        buf_window[0] = -1;
        buf_map[*src] = 0;
        pos = 1;
        for(src = state.first_undone; src < state.search_end;) {
            const ECL_usize limit_length = state.src_end - src;
            ECL_usize checked_idx, n_checks;
            state.n_new = src - state.first_undone;
            state.n_copy = 0;

            checked_idx = buf_map[*src];
            for(n_checks = 0; checked_idx != (ECL_usize)-1;) {
                ECL_usize curr_length;
                const uint8_t* const tmp_src2 = state.src_start + checked_idx;
                ECL_ASSERT(checked_idx < src_size);
                ++n_checks;
                for(curr_length = 1; curr_length < limit_length; ++curr_length) {
                    if(src[curr_length] != tmp_src2[curr_length]) {
                        break;
                    }
                }
                if(curr_length > state.n_copy) {
                    state.n_copy = curr_length;
                    state.offset = checked_idx;
                    if(curr_length == limit_length) {
                        break;
                    }
                }
                if(n_checks >= search_limit) {
                    break;
                }
                if((pos - checked_idx) > window_size) { // ran out of window
                    break;
                }
                checked_idx = buf_window[checked_idx & window_mask];
            }

            state.offset = pos - state.offset;
            if((state.n_copy > 1) && (*coder)(&state)) {
                uint8_t* const tmp = state.stream.next;
                ECL_JH_WJump(&state.stream, state.n_new);
                if(state.stream.is_valid) {
                    ECL_usize i;
                    if(state.n_new) {
                        memcpy(tmp, state.first_undone, state.n_new);
                    }
                    for(i = 0; i < state.n_copy; ++i, ++pos) {
                        buf_window[pos & window_mask] = buf_map[src[i]];
                        buf_map[src[i]] = pos;
                    }
                    src += state.n_copy;
                    state.first_undone = src;
                    continue;
                } else {
                    break;
                }
            }
            buf_window[pos & window_mask] = buf_map[*src];
            buf_map[*src] = pos;
            ++src;
            ++pos;
        }
    }
    return ECL_NanoLZ_CompleteCompression(&state, coder, dst);
}

ECL_usize ECL_NanoLZ_Compress_fast2(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit, ECL_NanoLZ_FastParams* p) {
    ECL_NanoLZ_CompressorState state;
    ECL_NanoLZ_SchemeCoder coder = ECL_NanoLZ_GetSchemeCoder(scheme);
    if((! coder) || (! p)) {
        return 0;
    }
    ECL_JH_WInit(&state.stream, dst, dst_size, 1);
    if((! src) || (! state.stream.is_valid)) {
        return 0;
    }
    memset(p->buf_map, -1, ECL_NANO_LZ_GET_FAST2_MAP_BUF_SIZE());
    {
        ECL_usize* const buf_map = (ECL_usize*)p->buf_map;
        ECL_usize* const buf_window = (ECL_usize*)p->buf_window;
        const ECL_usize window_size = 1UL << p->window_size_bits;
        const ECL_usize window_mask = (1UL << p->window_size_bits) - 1;
        ECL_usize pos;
        uint16_t key;

        *dst = *src; // copy first byte as is
        state.src_start = src;
        state.src_end = src + src_size;
        state.search_end = state.src_end - 1;
        state.first_undone = src + 1;
        key = ECL_READ_U16(src);
        buf_window[0] = -1;
        buf_map[key] = 0;
        pos = 1;
        for(src = state.first_undone; src < state.search_end;) {
            const ECL_usize limit_length = state.src_end - src;
            ECL_usize checked_idx, n_checks;
            state.n_new = src - state.first_undone;
            state.n_copy = 0;

            checked_idx = buf_map[ECL_READ_U16(src)];
            for(n_checks = 0; checked_idx != (ECL_usize)-1;) {
                ECL_usize curr_length;
                const uint8_t* const tmp_src2 = state.src_start + checked_idx;
                ECL_ASSERT(checked_idx < src_size);
                ++n_checks;
                for(curr_length = 2; curr_length < limit_length; ++curr_length) {
                    if(src[curr_length] != tmp_src2[curr_length]) {
                        break;
                    }
                }
                if(curr_length > state.n_copy) {
                    state.n_copy = curr_length;
                    state.offset = checked_idx;
                    if(state.n_copy == limit_length) {
                        break;
                    }
                }
                if(n_checks >= search_limit) {
                    break;
                }
                if((pos - checked_idx) > window_size) { // ran out of window
                    break;
                }
                checked_idx = buf_window[checked_idx & window_mask];
            }

            state.offset = pos - state.offset;
            if((state.n_copy > 1) && (*coder)(&state)) {
                uint8_t* const tmp = state.stream.next;
                ECL_JH_WJump(&state.stream, state.n_new);
                if(state.stream.is_valid) {
                    ECL_usize i;
                    if(state.n_new) {
                        memcpy(tmp, state.first_undone, state.n_new);
                    }
                    for(i = 0; i < state.n_copy; ++i, ++pos) {
                        key = ECL_READ_U16(src + i);
                        buf_window[pos & window_mask] = buf_map[key];
                        buf_map[key] = pos;
                    }
                    src += state.n_copy;
                    state.first_undone = src;
                    continue;
                } else {
                    break;
                }
            }
            key = ECL_READ_U16(src);
            buf_window[pos & window_mask] = buf_map[key];
            buf_map[key] = pos;
            ++src;
            ++pos;
        }
    }
    return ECL_NanoLZ_CompleteCompression(&state, coder, dst);
}

// -----------------------------------------------------------------------

ECL_usize ECL_NanoLZ_Decompress(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size) {
    ECL_NanoLZ_DecompressorState state;
    ECL_NanoLZ_SchemeDecoder decoder;
    ECL_usize dst_pos;

    ECL_NANO_LZ_COUNTER_CLEARALL();
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

#undef ECL_NANO_LZ_COUNTER_APPEND
#undef ECL_NANO_LZ_COUNTER_CLEARALL
