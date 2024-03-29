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

#include "ECL_ZeroEater.h"

typedef struct {
    const uint8_t* src;
    uint8_t* dst;
    uint8_t* dst_end;
    ECL_usize result_size;
} ECL_ZeroEaterComprssorState;

static void ECL_ZeroEater_DumpSeq1(ECL_ZeroEaterComprssorState* state, ECL_usize count_x) {
    ECL_usize full_blocks, part_block_cnt, length;
    full_blocks = count_x >> 7;
    part_block_cnt = count_x & 0x007F;
    length = full_blocks;
    if(part_block_cnt) {
        ++length;
    }
    length += count_x;
    state->result_size += length;
    if((state->dst + length) <= state->dst_end) {
        for(; full_blocks > 0; --full_blocks) {
            *(state->dst) = 0x7F; /* opcode */
            memcpy(state->dst + 1, state->src, 128);
            state->src += 128;
            state->dst += 1 + 128;
        }
        if(part_block_cnt) {
            *(state->dst) = (uint8_t)part_block_cnt - 1; /* opcode */
            memcpy(state->dst + 1, state->src, part_block_cnt);
            state->src += part_block_cnt;
            state->dst += 1 + part_block_cnt;
        }
    }
}

static void ECL_ZeroEater_DumpSeq2(ECL_ZeroEaterComprssorState* state, ECL_usize count_0) {
    ECL_usize full_blocks, part_block_cnt, length;
    full_blocks = count_0 >> 6;
    part_block_cnt = count_0 & 0x003F;
    length = full_blocks;
    if(part_block_cnt) {
        ++length;
    }
    state->result_size += length;
    if((state->dst + length) <= state->dst_end) {
        for(; full_blocks > 0; --full_blocks) {
            *(state->dst) = 0xBF; /* opcode */
            ++(state->dst);
        }
        if(part_block_cnt) {
            *(state->dst) = (uint8_t)part_block_cnt - 0x81; /* subtract 1, subtract extra 0x80 to get 1 in higher bit */
            ++(state->dst);
        }
    }
}

static void ECL_ZeroEater_DumpSeq3(ECL_ZeroEaterComprssorState* state, ECL_usize count_x, ECL_usize count_0) {
    /* count_x = [1..8]; count_0 = [1..8]; */
    ECL_SCOPED_CONST ECL_usize length = count_x + 1;
    state->result_size += length;
    if((state->dst + length) <= state->dst_end) {
        *(state->dst) = ((uint8_t)count_0 - (uint8_t)0x41) | (((uint8_t)count_x - (uint8_t)1) << 3); /* subtract extra 0x40 to get 11 in higher bits */
        memcpy(state->dst + 1, state->src, count_x);
        state->src += count_x;
        state->dst += 1 + count_x;
    }
}

static void ECL_ZeroEater_DumpGeneric(ECL_ZeroEaterComprssorState* state, ECL_usize count_x, ECL_usize count_0) {
    /* count_x > 0; count_0 > 0 */
    ECL_SCOPED_CONST ECL_usize x_full_blocks = count_x >> 7;
    if(x_full_blocks) {
        ECL_SCOPED_CONST ECL_usize tmp = count_x & 0x7F; /* last block size */
        ECL_ZeroEater_DumpSeq1(state, count_x - tmp);
        count_x = tmp;
    }
    if(count_x && (count_x < 9) && (count_0 < 9)) {
        ECL_ZeroEater_DumpSeq3(state, count_x, count_0);
    } else if(count_x && (count_x < 9)) {
        ECL_ZeroEater_DumpSeq3(state, count_x, 8);
        ECL_ZeroEater_DumpSeq2(state, count_0 - 8);
    } else {
        if(count_x) {
            ECL_ZeroEater_DumpSeq1(state, count_x);
        }
        ECL_ZeroEater_DumpSeq2(state, count_0);
    }
}

ECL_usize ECL_ZeroEater_Compress(const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size) {
    ECL_ZeroEaterComprssorState state;
    ECL_usize cnt_x, cnt_0;
    const uint8_t* first_x;
    const uint8_t* first_0;
    const uint8_t* ECL_SCOPED_CONST src_end = src + src_size;
    if(! src) {
        return 0;
    }
    if(! dst) {
        dst_size = 0;
    }
    state.src = src;
    state.result_size = 0;
    state.dst = dst;
    state.dst_end = dst ? (dst + dst_size) : dst;
    first_x = src;
    while(state.src < src_end) {
        for(first_0 = first_x; (first_0 < src_end) && *first_0; ++first_0); /* search for zero from where last search ended */

        cnt_x = first_0 - state.src; /* count of non-zeroes in beginning */
        if(first_0 == src_end) { /* complete it (1) */
            ECL_ZeroEater_DumpSeq1(&state, cnt_x);
            break;
        }
        /* we found zero, find next non-zero */
        for(first_x = first_0; (first_x < src_end) && (! *first_x); ++first_x);

        cnt_0 = first_x - first_0; /* count of zeroes afterwards */
        /* stream looks like {state.src: [xx..x] first_0: [00..0] first_x: ...} */
        if(cnt_x) { /* has non-zero stream too (3) */
            if((cnt_0 == 1) && (cnt_x > 8)) { /* bad deal */
                /* this particular case is the worst, this single zero-byte should be considered as non-zero for better compression */
                /* this data will be processed in next iteration */
                continue;
            }
            ECL_ZeroEater_DumpGeneric(&state, cnt_x, cnt_0);
            state.src = first_x;
        } else { /* only zero stream (2) */
            ECL_ZeroEater_DumpSeq2(&state, cnt_0);
            state.src = first_x;
        }
    }
    return state.result_size;
}

ECL_usize ECL_ZeroEater_Decompress(const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size) {
    const uint8_t* src_end = src + src_size;
    const uint8_t* dst_start = dst;
    const uint8_t* dst_end = dst + dst_size;
    if((! src) || (! dst)) { /* invalid parameters */
        return 0;
    }
    for(; src < src_end; ) {
        ECL_usize cnt_x, cnt_0;
        ECL_SCOPED_CONST uint8_t opcode = *src;
        ++src;
        if(opcode & 0x80) { /* method 2 or 3 */
            if(opcode & 0x40) { /* method 3 */
                cnt_x = ((opcode >> 3) & 0x07) + 1;
                cnt_0 = (opcode & 0x07) + 1;
                /* check if fits output buffer and unpack */
                if((cnt_x + cnt_0) > (ECL_usize)(dst_end - dst)) {
                    break;
                }
                if(cnt_x > (ECL_usize)(src_end - src)) {
                    break; /* invalid stream */
                }
                memcpy(dst, src, cnt_x);
                memset(dst + cnt_x, 0, cnt_0);
                dst += cnt_x + cnt_0;
                src += cnt_x;
            } else { /* method 2 */
                cnt_0 = (opcode & 0x3F) + 1;
                if(cnt_0 > (ECL_usize)(dst_end - dst)) {
                    return 0;
                }
                memset(dst, 0, cnt_0);
                dst += cnt_0;
            }
        } else { /* method 1 */
            cnt_x = opcode + 1;
            if(cnt_x > (ECL_usize)(dst_end - dst)) {
                break;
            }
            if(cnt_x > (ECL_usize)(src_end - src)) {
                break; /* invalid stream */
            }
            memcpy(dst, src, cnt_x);
            dst += cnt_x;
            src += cnt_x;
        }
    }
    return (src == src_end) ? (dst - dst_start) : 0; /* ensure all source is consumed */
}
