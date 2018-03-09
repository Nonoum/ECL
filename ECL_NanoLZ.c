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

#include "ECL_NanoLZ.h"
#include "ECL_JH_States.h"

typedef struct {
    const uint8_t* src_start;
    const uint8_t* src_end;
    const uint8_t* search_end;
    const uint8_t* first_undone;

    ECL_JH_WState stream;
    ECL_usize n_new; /* amount of new raw values */
    ECL_ssize offset; /* offset / distance */
    ECL_usize n_copy; /* amount of repeated values to copy */
} ECL_NanoLZ_CompressorState;

typedef struct {
    ECL_JH_RState stream;
    ECL_usize n_new;
    ECL_ssize offset;
    ECL_usize n_copy;
} ECL_NanoLZ_DecompressorState;

/*
    ECL_NanoLZ_SchemeCoder is a pointer to coder function.
    Coder function returns whether code is applied (worth applying).

    ECL_NanoLZ_SchemeDecoder is a pointer to decoder function.
*/
typedef bool(*ECL_NanoLZ_SchemeCoder)(ECL_NanoLZ_CompressorState*);
typedef void(*ECL_NanoLZ_SchemeDecoder)(ECL_NanoLZ_DecompressorState*);

/* import schemes, define counters if needed */
#ifdef ECL_USE_STAT_COUNTERS
    int ECL_NanoLZ_Decompression_OpcodePickCounters[ECL_NANO_LZ_DECOMPRESSION_OPCODE_PICK_COUNTERS_COUNT];
    #define ECL_NANO_LZ_COUNTER_APPEND(index, value) ECL_NanoLZ_Decompression_OpcodePickCounters[index] += value;
    #define ECL_NANO_LZ_COUNTER_CLEARALL() memset(ECL_NanoLZ_Decompression_OpcodePickCounters, 0, sizeof(ECL_NanoLZ_Decompression_OpcodePickCounters));
#else
    #define ECL_NANO_LZ_COUNTER_APPEND(index, value)
    #define ECL_NANO_LZ_COUNTER_CLEARALL()
#endif

#include "ECL_NanoLZ_schemes_inline.h"

/* ECL_ARE_U16S_EQUAL safely compares two pairs of bytes at specified uint8_t addresses */
#define ECL_ARE_U16S_EQUAL(ubyte_addr1, ubyte_addr2) ( ((*(ubyte_addr1)) == (*(ubyte_addr2))) && (( (ubyte_addr1)[1] ) == ( (ubyte_addr2)[1] )) )

/* ECL_READ_U16_WHATEVER safely reads uint16_t value from an uint8_t memory pointer in whatever consistent Endianness */
#define ECL_READ_U16_WHATEVER(ubyte_addr) ((((uint16_t)*(ubyte_addr)) << 8) | ((ubyte_addr)[1]))


static bool ECL_NanoLZ_SetupCompression(ECL_NanoLZ_CompressorState* s, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size) {
    ECL_JH_WInit(&s->stream, dst, dst_size, 1);
    if((! src) || (! src_size) || (! s->stream.is_valid)) {
        return false;
    }
    s->src_start = src;
    s->src_end = src + src_size;
    s->search_end = s->src_end - 1;
    s->first_undone = src + 1;
    *dst = *src; /* copy first byte as is */
    return true;
}

static bool ECL_NanoLZ_ValidateU16size(ECL_usize src_size) {
    if(src_size > 0x0FFFE) {
        return false;
    }
    return true;
}

static ECL_usize ECL_NanoLZ_CompleteCompression(ECL_NanoLZ_CompressorState* s, ECL_NanoLZ_Scheme scheme, const uint8_t* dst_start) {
    s->n_new = s->src_end - s->first_undone;
    if(s->stream.is_valid && s->n_new) {
        uint8_t* tmp;
        ECL_NanoLZ_SchemeCoder coder;
        s->n_copy = 0;
        coder = ECL_NanoLZ_GetSchemeCoderNoCopy(scheme);
        if(! coder) {
            return 0;
        }
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
    const ECL_NanoLZ_SchemeCoder coder = ECL_NanoLZ_GetSchemeCoder(scheme);
    if((! coder) || (! ECL_NanoLZ_SetupCompression(&state, src, src_size, dst, dst_size))) {
        return 0;
    }
    for(src = state.first_undone; src < state.search_end;) {
        const uint8_t* const back_search_end = src - ECL_MIN((ECL_usize)(src - state.src_start), search_limit);
        const uint8_t* candidate = src;
        const ECL_usize limit_length = state.src_end - src;
        state.n_copy = 0;
        for(--candidate; candidate >= back_search_end; --candidate) {
            ECL_usize curr_length;
            if(! ECL_ARE_U16S_EQUAL(src, candidate)) {
                continue;
            }
            for(curr_length = 2; curr_length < limit_length; ++curr_length) {
                if(src[curr_length] != candidate[curr_length]) {
                    break;
                }
            }
            if(curr_length > state.n_copy) {
                state.n_copy = curr_length;
                state.offset = src - candidate;
                if(state.n_copy == limit_length) {
                    break;
                }
            }
        }
        if(state.n_copy > 1) {
            state.n_new = src - state.first_undone;
            if((*coder)(&state)) {
                uint8_t* const tmp = state.stream.next;
                ECL_JH_WJump(&state.stream, state.n_new);
                if(state.stream.is_valid) {
                    ECL_usize i;
                    for(i = 0; i < state.n_new; ++i) { /* memcpy is inefficient here */
                        tmp[i] = state.first_undone[i];
                    }
                    src += state.n_copy;
                    state.first_undone = src;
                    continue;
                } else {
                    break;
                }
            }
        }
        ++src;
    }
    /* main cycle done. dump last seq */
    return ECL_NanoLZ_CompleteCompression(&state, scheme, dst);
}

/* 'mid' versions -------------------------------------------------------- */

ECL_usize ECL_NanoLZ_Compress_mid1(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit, void* buf_256) {
    ECL_NanoLZ_CompressorState state;
    const ECL_NanoLZ_SchemeCoder coder = ECL_NanoLZ_GetSchemeCoder(scheme);
    const ECL_usize window_length = (src_size >> 1) + (src_size & 1); /* half, round up */
    if(! ECL_NanoLZ_ValidateU16size(src_size)) {
        return 0; /* not allowed for this mode */
    }
    if(window_length >= dst_size) {
        return 0; /* not allowed for this mode */
    }
    if((! buf_256) || (! coder) || (! ECL_NanoLZ_SetupCompression(&state, src, src_size, dst, dst_size))) {
        return 0;
    }
    memset(buf_256, 0, 256);
    {
        uint8_t* const buf_map = (uint8_t*)buf_256;
        uint8_t* const buf_window = dst + dst_size - window_length;
        ECL_usize pos = 1;

        memset(buf_window, 0, window_length); /* fill with zeroes */
        for(src = state.first_undone; src < state.search_end;) {
            const ECL_usize limit_length = state.src_end - src;
            ECL_usize checked_idx, n_checks;
            state.n_copy = 0;

            checked_idx = ((ECL_usize)buf_map[*src]) | (pos & ~(ECL_usize)0x0FF);
            /* catch up index */
            if(checked_idx >= pos) {
                checked_idx -= 256;
                ECL_ASSERT(checked_idx < src_size); /* can't go < 0. check as unsigned */
            }
            while(state.src_start[checked_idx] != *src) {
                if(checked_idx >= 256) {
                    checked_idx -= 256;
                } else {
                    break;
                }
            }
            if(state.src_start[checked_idx] == *src) { /* found minimal match */
                for(n_checks = 0; ; ) {
                    ECL_usize curr_length;
                    const uint8_t* const tmp_src2 = state.src_start + checked_idx;
                    ECL_ASSERT(checked_idx < src_size);
                    ECL_ASSERT(*src == *tmp_src2);
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
                    /* pick next match */
                    {
                        const ECL_usize table_index = checked_idx >> 1;
                        const ECL_usize last_checked = checked_idx;
                        ECL_ASSERT(last_checked <= pos);
                        if((buf_window + table_index) < state.stream.next) {
                            break; /* window data is invalid there */
                        }
                        checked_idx = ((buf_window[table_index] >> ((last_checked & 1) * 4)) & 0x0F)
                                    | (last_checked & ~(ECL_usize)0x0F);
                        /* catch up index */
                        if(checked_idx >= last_checked) {
                            if(! checked_idx) {
                                break; /* no more matches */
                            }
                            checked_idx -= 16;
                            ECL_ASSERT(checked_idx < src_size); /* can't go < 0. check as unsigned */
                        }
                        while(state.src_start[checked_idx] != *src) {
                            if(checked_idx >= 16) {
                                checked_idx -= 16;
                            } else {
                                break;
                            }
                        }
                        if(state.src_start[checked_idx] != *src) {
                            break; /* no more matches */
                        }
                    }
                }

                if(state.n_copy > 1) {
                    state.offset = pos - state.offset;
                    state.n_new = src - state.first_undone;
                    if((*coder)(&state)) {
                        uint8_t* const tmp = state.stream.next;
                        ECL_JH_WJump(&state.stream, state.n_new);
                        if(state.stream.is_valid) {
                            ECL_usize i;
                            for(i = 0; i < state.n_new; ++i) { /* memcpy is inefficient here */
                                tmp[i] = state.first_undone[i];
                            }
                            /* update tables for referenced sequence */
                            for(i = 0; i < state.n_copy; ++i, ++pos) {
                                const ECL_usize table_index = pos >> 1;
                                if((buf_window + table_index) >= state.stream.next) {
                                    const uint8_t prev_pos = buf_map[src[i]];
                                    buf_window[table_index] |= (prev_pos & 0x0F) << ((pos & 1) * 4);
                                }
                                buf_map[src[i]] = pos;
                            }
                            src += state.n_copy;
                            state.first_undone = src;
                            continue;
                        } else {
                            break;
                        }
                    }
                }
            }
            {
                /* update tables for passed byte */
                const ECL_usize table_index = pos >> 1;
                if((buf_window + table_index) >= state.stream.next) {
                    const uint8_t prev_pos = buf_map[*src];
                    buf_window[table_index] |= (prev_pos & 0x0F) << ((pos & 1) * 4);
                }
                buf_map[*src] = pos;
            }
            ++src;
            ++pos;
        }
    }
    return ECL_NanoLZ_CompleteCompression(&state, scheme, dst);
}

ECL_usize ECL_NanoLZ_Compress_mid2(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit, void* buf_513) {
    ECL_NanoLZ_CompressorState state;
    const ECL_NanoLZ_SchemeCoder coder = ECL_NanoLZ_GetSchemeCoder(scheme);
    const ECL_usize window_length = (src_size >> 1) + (src_size & 1); /* half, round up */
    if(! ECL_NanoLZ_ValidateU16size(src_size)) {
        return 0; /* not allowed for this mode */
    }
    if(window_length >= dst_size) {
        return 0; /* not allowed for this mode */
    }
    if((! buf_513) || (! coder) || (! ECL_NanoLZ_SetupCompression(&state, src, src_size, dst, dst_size))) {
        return 0;
    }
    memset(buf_513, 0, 513);
    {
        uint16_t* const buf_map = ECL_GetAlignedPointer2((uint8_t*)buf_513);
        uint8_t* const buf_window = dst + dst_size - window_length;
        ECL_usize pos = 1;

        memset(buf_window, 0, window_length); /* fill with zeroes */
        for(src = state.first_undone; src < state.search_end;) {
            const ECL_usize limit_length = state.src_end - src;
            ECL_usize checked_idx, n_checks;
            state.n_copy = 0;

            checked_idx = buf_map[*src];
            if(state.src_start[checked_idx] == *src) { /* found minimal match */
                for(n_checks = 0; ; ) {
                    ECL_usize curr_length;
                    const uint8_t* const tmp_src2 = state.src_start + checked_idx;
                    ECL_ASSERT(checked_idx < src_size);
                    ECL_ASSERT(*src == *tmp_src2);
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
                    /* pick next match */
                    {
                        const ECL_usize table_index = checked_idx >> 1;
                        const ECL_usize last_checked = checked_idx;
                        ECL_ASSERT(last_checked <= pos);
                        if((buf_window + table_index) < state.stream.next) {
                            break; /* window data is invalid there */
                        }
                        checked_idx = ((buf_window[table_index] >> ((last_checked & 1) * 4)) & 0x0F)
                                    | (last_checked & ~(ECL_usize)0x0F);
                        /* catch up index */
                        if(checked_idx >= last_checked) {
                            if(! checked_idx) {
                                break; /* no more matches */
                            }
                            checked_idx -= 16;
                            ECL_ASSERT(checked_idx < src_size); /* can't go < 0. check as unsigned */
                        }
                        while(state.src_start[checked_idx] != *src) {
                            if(checked_idx >= 16) {
                                checked_idx -= 16;
                            } else {
                                break;
                            }
                        }
                        if(state.src_start[checked_idx] != *src) {
                            break; /* no more matches */
                        }
                    }
                }

                if(state.n_copy > 1) {
                    state.offset = pos - state.offset;
                    state.n_new = src - state.first_undone;
                    if((*coder)(&state)) {
                        uint8_t* const tmp = state.stream.next;
                        ECL_JH_WJump(&state.stream, state.n_new);
                        if(state.stream.is_valid) {
                            ECL_usize i;
                            for(i = 0; i < state.n_new; ++i) { /* memcpy is inefficient here */
                                tmp[i] = state.first_undone[i];
                            }
                            /* update tables for referenced sequence */
                            for(i = 0; i < state.n_copy; ++i, ++pos) {
                                const ECL_usize table_index = pos >> 1;
                                if((buf_window + table_index) >= state.stream.next) {
                                    const uint8_t prev_pos = buf_map[src[i]];
                                    buf_window[table_index] |= (prev_pos & 0x0F) << ((pos & 1) * 4);
                                }
                                buf_map[src[i]] = pos;
                            }
                            src += state.n_copy;
                            state.first_undone = src;
                            continue;
                        } else {
                            break;
                        }
                    }
                }
            }
            {
                /* update tables for passed byte */
                const ECL_usize table_index = pos >> 1;
                if((buf_window + table_index) >= state.stream.next) {
                    const uint8_t prev_pos = buf_map[*src];
                    buf_window[table_index] |= (prev_pos & 0x0F) << ((pos & 1) * 4);
                }
                buf_map[*src] = pos;
            }
            ++src;
            ++pos;
        }
    }
    return ECL_NanoLZ_CompleteCompression(&state, scheme, dst);
}

/* mid*min versions */

ECL_usize ECL_NanoLZ_Compress_mid1min(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, void* buf_256) {
    ECL_NanoLZ_CompressorState state;
    const ECL_NanoLZ_SchemeCoder coder = ECL_NanoLZ_GetSchemeCoder(scheme);
    if(! ECL_NanoLZ_ValidateU16size(src_size)) {
        return 0; /* not allowed for this mode */
    }
    if((! buf_256) || (! coder) || (! ECL_NanoLZ_SetupCompression(&state, src, src_size, dst, dst_size))) {
        return 0;
    }
    memset(buf_256, 0, 256);
    {
        uint8_t* const buf_map = (uint8_t*)buf_256;
        ECL_usize pos = 1;
        for(src = state.first_undone; src < state.search_end;) {
            const ECL_usize limit_length = state.src_end - src;
            ECL_usize checked_idx = ((ECL_usize)buf_map[*src]) | (pos & ~(ECL_usize)0x0FF);
            /* catch up index */
            if(checked_idx >= pos) {
                checked_idx -= 256;
                ECL_ASSERT(checked_idx < src_size); /* can't go < 0. check as unsigned */
            }
            while(state.src_start[checked_idx] != *src) {
                if(checked_idx >= 256) {
                    checked_idx -= 256;
                } else {
                    break;
                }
            }
            {
                const uint8_t* const tmp_src2 = state.src_start + checked_idx;
                if(ECL_ARE_U16S_EQUAL(src, tmp_src2)) { /* found minimal match */
                    ECL_usize curr_length;
                    for(curr_length = 2; curr_length < limit_length; ++curr_length) {
                        if(src[curr_length] != tmp_src2[curr_length]) {
                            break;
                        }
                    }
                    state.n_copy = curr_length;
                    state.offset = pos - checked_idx;
                    state.n_new = src - state.first_undone;
                    if((*coder)(&state)) {
                        uint8_t* const tmp = state.stream.next;
                        ECL_JH_WJump(&state.stream, state.n_new);
                        if(state.stream.is_valid) {
                            ECL_usize i;
                            for(i = 0; i < state.n_new; ++i) { /* memcpy is inefficient here */
                                tmp[i] = state.first_undone[i];
                            }
                            /* update tables for referenced sequence */
                            for(i = 0; i < state.n_copy; ++i, ++pos) {
                                buf_map[src[i]] = pos;
                            }
                            src += state.n_copy;
                            state.first_undone = src;
                            continue;
                        } else {
                            break;
                        }
                    }
                }
            }
            buf_map[*src] = pos;
            ++src;
            ++pos;
        }
    }
    return ECL_NanoLZ_CompleteCompression(&state, scheme, dst);
}

ECL_usize ECL_NanoLZ_Compress_mid2min(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, void* buf_513) {
    ECL_NanoLZ_CompressorState state;
    const ECL_NanoLZ_SchemeCoder coder = ECL_NanoLZ_GetSchemeCoder(scheme);
    if(! ECL_NanoLZ_ValidateU16size(src_size)) {
        return 0; /* not allowed for this mode */
    }
    if((! buf_513) || (! coder) || (! ECL_NanoLZ_SetupCompression(&state, src, src_size, dst, dst_size))) {
        return 0;
    }
    memset(buf_513, 0, 513);
    {
        uint16_t* const buf_map = ECL_GetAlignedPointer2((uint8_t*)buf_513);
        ECL_usize pos = 1;
        for(src = state.first_undone; src < state.search_end;) {
            const ECL_usize limit_length = state.src_end - src;
            const ECL_usize checked_idx = buf_map[*src];
            const uint8_t* const tmp_src2 = state.src_start + checked_idx;
            if(ECL_ARE_U16S_EQUAL(src, tmp_src2)) { /* found minimal match */
                ECL_usize curr_length;
                for(curr_length = 2; curr_length < limit_length; ++curr_length) {
                    if(src[curr_length] != tmp_src2[curr_length]) {
                        break;
                    }
                }
                state.n_copy = curr_length;
                state.offset = pos - checked_idx;
                state.n_new = src - state.first_undone;
                if((*coder)(&state)) {
                    uint8_t* const tmp = state.stream.next;
                    ECL_JH_WJump(&state.stream, state.n_new);
                    if(state.stream.is_valid) {
                        ECL_usize i;
                        for(i = 0; i < state.n_new; ++i) { /* memcpy is inefficient here */
                            tmp[i] = state.first_undone[i];
                        }
                        /* update tables for referenced sequence */
                        for(i = 0; i < state.n_copy; ++i, ++pos) {
                            buf_map[src[i]] = pos;
                        }
                        src += state.n_copy;
                        state.first_undone = src;
                        continue;
                    } else {
                        break;
                    }
                }
            }
            buf_map[*src] = pos;
            ++src;
            ++pos;
        }
    }
    return ECL_NanoLZ_CompleteCompression(&state, scheme, dst);
}

/* 'fast' versions ------------------------------------------------------------------------------ */

static void ECL_NanoLZ_FastParams_Cleanup(ECL_NanoLZ_FastParams* p) {
    /* avoid memset, just in case */
    p->buf_map = 0;
    p->buf_window = 0;
    p->window_size_bits = 0;
}

static bool ECL_NanoLZ_FastParams_Validate(ECL_NanoLZ_FastParams* p) {
    return p && (p->buf_map) && (p->buf_window);
}

static bool ECL_NanoLZ_FastParams_Alloc(ECL_NanoLZ_FastParams* p, uint8_t ver, uint8_t window_size_bits) {
    if(! p) {
        return false;
    }
    ECL_NanoLZ_FastParams_Cleanup(p);
    if((! window_size_bits) || (window_size_bits >= ( (sizeof(long)*8) - sizeof(ECL_usize) ))) {
        return false;
    }
    if(ver == 1) {
        p->buf_map = ECL_MEM_ALLOC(ECL_NANO_LZ_GET_FAST1_MAP_BUF_SIZE());
        p->buf_window = ECL_MEM_ALLOC(ECL_NANO_LZ_GET_FAST1_WINDOW_BUF_SIZE(window_size_bits));
    } else if(ver == 2) {
        p->buf_map = ECL_MEM_ALLOC(ECL_NANO_LZ_GET_FAST2_MAP_BUF_SIZE());
        p->buf_window = ECL_MEM_ALLOC(ECL_NANO_LZ_GET_FAST2_WINDOW_BUF_SIZE(window_size_bits));
    }
    p->window_size_bits = window_size_bits;
    if((p->buf_map) && (p->buf_window)) {
        return true;
    }
    ECL_NanoLZ_FastParams_Destroy(p);
    return false;
}

bool ECL_NanoLZ_FastParams_Alloc1(ECL_NanoLZ_FastParams* p, uint8_t window_size_bits) {
    return ECL_NanoLZ_FastParams_Alloc(p, 1, window_size_bits);
}

bool ECL_NanoLZ_FastParams_Alloc2(ECL_NanoLZ_FastParams* p, uint8_t window_size_bits) {
    return ECL_NanoLZ_FastParams_Alloc(p, 2, window_size_bits);
}

void ECL_NanoLZ_FastParams_Destroy(ECL_NanoLZ_FastParams* p) {
    ECL_MEM_FREE(p->buf_map);
    ECL_MEM_FREE(p->buf_window);
    ECL_NanoLZ_FastParams_Cleanup(p);
}

ECL_usize ECL_NanoLZ_Compress_fast1(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit, ECL_NanoLZ_FastParams* p) {
    ECL_NanoLZ_CompressorState state;
    const ECL_NanoLZ_SchemeCoder coder = ECL_NanoLZ_GetSchemeCoder(scheme);
    if((! coder) || (! ECL_NanoLZ_SetupCompression(&state, src, src_size, dst, dst_size))) {
        return 0;
    }
    if(! ECL_NanoLZ_FastParams_Validate(p)) {
        return 0;
    }
    memset(p->buf_map, -1, ECL_NANO_LZ_GET_FAST1_MAP_BUF_SIZE());
    {
        ECL_usize* const buf_map = ECL_GetAlignedPointerS((uint8_t*)(p->buf_map));
        ECL_usize* const buf_window = ECL_GetAlignedPointerS((uint8_t*)(p->buf_window));
        const ECL_usize window_size = ((ECL_usize)1) << p->window_size_bits;
        const ECL_usize window_mask = (((ECL_usize)1) << p->window_size_bits) - 1;
        ECL_usize pos = 1;

        buf_window[0] = (ECL_usize)(-1);
        buf_map[*src] = 0;
        for(src = state.first_undone; src < state.search_end;) {
            const ECL_usize limit_length = state.src_end - src;
            ECL_usize checked_idx, n_checks;
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
                if((pos - checked_idx) > window_size) { /* ran out of window */
                    break;
                }
                checked_idx = buf_window[checked_idx & window_mask];
            }

            if(state.n_copy > 1) {
                state.offset = pos - state.offset;
                state.n_new = src - state.first_undone;
                if((*coder)(&state)) {
                    uint8_t* const tmp = state.stream.next;
                    ECL_JH_WJump(&state.stream, state.n_new);
                    if(state.stream.is_valid) {
                        ECL_usize i;
                        for(i = 0; i < state.n_new; ++i) { /* memcpy is inefficient here */
                            tmp[i] = state.first_undone[i];
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
            }
            buf_window[pos & window_mask] = buf_map[*src];
            buf_map[*src] = pos;
            ++src;
            ++pos;
        }
    }
    return ECL_NanoLZ_CompleteCompression(&state, scheme, dst);
}

ECL_usize ECL_NanoLZ_Compress_fast2(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit, ECL_NanoLZ_FastParams* p) {
    ECL_NanoLZ_CompressorState state;
    const ECL_NanoLZ_SchemeCoder coder = ECL_NanoLZ_GetSchemeCoder(scheme);
    if((! coder) || (! ECL_NanoLZ_SetupCompression(&state, src, src_size, dst, dst_size))) {
        return 0;
    }
    if(! ECL_NanoLZ_FastParams_Validate(p)) {
        return 0;
    }
    memset(p->buf_map, -1, ECL_NANO_LZ_GET_FAST2_MAP_BUF_SIZE());
    {
        ECL_usize* const buf_map = ECL_GetAlignedPointerS((uint8_t*)(p->buf_map));
        ECL_usize* const buf_window = ECL_GetAlignedPointerS((uint8_t*)(p->buf_window));
        const ECL_usize window_size = ((ECL_usize)1) << p->window_size_bits;
        const ECL_usize window_mask = (((ECL_usize)1) << p->window_size_bits) - 1;
        ECL_usize pos;
        uint16_t key;

        key = ECL_READ_U16_WHATEVER(src);
        buf_window[0] = (ECL_usize)(-1);
        buf_map[key] = 0;
        pos = 1;
        for(src = state.first_undone; src < state.search_end;) {
            const ECL_usize limit_length = state.src_end - src;
            ECL_usize checked_idx, n_checks;
            state.n_copy = 0;

            checked_idx = buf_map[ECL_READ_U16_WHATEVER(src)];
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
                if((pos - checked_idx) > window_size) { /* ran out of window */
                    break;
                }
                checked_idx = buf_window[checked_idx & window_mask];
            }

            if(state.n_copy > 1) {
                state.offset = pos - state.offset;
                state.n_new = src - state.first_undone;
                if((*coder)(&state)) {
                    uint8_t* const tmp = state.stream.next;
                    ECL_JH_WJump(&state.stream, state.n_new);
                    if(state.stream.is_valid) {
                        ECL_usize i;
                        for(i = 0; i < state.n_new; ++i) { /* memcpy is inefficient here */
                            tmp[i] = state.first_undone[i];
                        }
                        for(i = 0; i < state.n_copy; ++i, ++pos) {
                            key = ECL_READ_U16_WHATEVER(src + i);
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
            }
            key = ECL_READ_U16_WHATEVER(src);
            buf_window[pos & window_mask] = buf_map[key];
            buf_map[key] = pos;
            ++src;
            ++pos;
        }
    }
    return ECL_NanoLZ_CompleteCompression(&state, scheme, dst);
}

/* ----------------------------------------------------------------------- */

ECL_usize ECL_NanoLZ_Compress_auto(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit) {
    return ECL_NanoLZ_Compress_auto_ex(scheme, src, src_size, dst, dst_size, search_limit, 0, 0);
}

ECL_usize ECL_NanoLZ_Compress_auto_ex(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit
                                      , ECL_NanoLZ_FastParams* prealloc1, ECL_NanoLZ_FastParams* prealloc2)
{
    ECL_usize result = 0;
    if((search_limit < 2) && ECL_NanoLZ_ValidateU16size(src_size)) { /* for minimal limit mid2min is fastest choice */
        uint8_t buf[513];
        result = ECL_NanoLZ_Compress_mid2min(scheme, src, src_size, dst, dst_size, buf);
    } else if(src_size < 70) { /* prefer mid2 */
        uint8_t buf[513];
        uint8_t tmp[ECL_NANO_LZ_MID_GET_BOUND_OPTIMAL(70)];
        result = ECL_NanoLZ_Compress_mid2(scheme, src, src_size, tmp, sizeof(tmp), search_limit, buf);
        if(result && (result <= dst_size)) {
            memcpy(dst, tmp, result);
        }
    } else if(src_size < 1500) { /* prefer fast1 */
        if(prealloc1) {
            result = ECL_NanoLZ_Compress_fast1(scheme, src, src_size, dst, dst_size, search_limit, prealloc1);
        } else {
            ECL_NanoLZ_FastParams fp;
            if(ECL_NanoLZ_FastParams_Alloc1(&fp, ECL_LogRoundUp(src_size))) {
                result = ECL_NanoLZ_Compress_fast1(scheme, src, src_size, dst, dst_size, search_limit, &fp);
                ECL_NanoLZ_FastParams_Destroy(&fp);
            }
        }
    } else { /* prefer fast2 */
        if(prealloc2) {
            result = ECL_NanoLZ_Compress_fast2(scheme, src, src_size, dst, dst_size, search_limit, prealloc2);
        } else {
            ECL_NanoLZ_FastParams fp;
            if(ECL_NanoLZ_FastParams_Alloc2(&fp, ECL_LogRoundUp(src_size))) {
                result = ECL_NanoLZ_Compress_fast2(scheme, src, src_size, dst, dst_size, search_limit, &fp);
                ECL_NanoLZ_FastParams_Destroy(&fp);
            }
        }
    }
    return result;
}

/* ----------------------------------------------------------------------- */

ECL_usize ECL_NanoLZ_Decompress(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size) {
    ECL_NanoLZ_DecompressorState state;
    ECL_NanoLZ_SchemeDecoder decoder;
    uint8_t* const dst_start = dst;
    uint8_t* const dst_end = dst + dst_size;

    ECL_NANO_LZ_COUNTER_CLEARALL();
    decoder = ECL_NanoLZ_GetSchemeDecoder(scheme);
    if(! decoder) {
        return 0;
    }
    ECL_JH_RInit(&state.stream, src, src_size, 1);
    if((! dst) || (! dst_size) || (! state.stream.is_valid)) {
        return 0;
    }
    *dst = *src; /* copy first byte as is */
    for(++dst; ; ) {
        const ECL_usize left = dst_end - dst;
        if(! left) {
            break; /* ok - done */
        }
        (*decoder)(&state);
        if(! state.stream.is_valid) {
            break; /* error in stream */
        }
        if((state.n_new + state.n_copy) > left) {
            state.stream.is_valid = 0;
            break; /* output doesn't fit this */
        }
        if(state.n_new) {
            const uint8_t* const src_block_start = state.stream.next;
            ECL_usize i = 0;
            ECL_JH_RJump(&state.stream, state.n_new);
            if(! state.stream.is_valid) {
                break; /* don't have enough data in stream */
            }
            for(; i < state.n_new; ++i) { /* memcpy is inefficient here */
                dst[i] = src_block_start[i];
            }
            dst += state.n_new;
        }
        if(state.n_copy) {
            if(state.offset == 1) {
                memset(dst, dst[-1], state.n_copy); /* memset wins against cycle in average case */
                dst += state.n_copy;
            } else {
                const ECL_ssize offs = state.offset;
                ECL_usize i;
                if(offs > (dst - dst_start)) {
                    state.stream.is_valid = 0;
                    break; /* points outside of data. error in stream */
                }
                i = state.n_copy;
                for(; i; --i, ++dst) { /* memcpy is inefficient here */
                    *dst = *(dst - offs);
                }
            }
        }
    }
    return ((state.stream.next == state.stream.end) && state.stream.is_valid) ? (dst - dst_start) : 0;
}

#undef ECL_NANO_LZ_COUNTER_APPEND
#undef ECL_NANO_LZ_COUNTER_CLEARALL
#undef ECL_ARE_U16S_EQUAL
#undef ECL_READ_U16_WHATEVER
