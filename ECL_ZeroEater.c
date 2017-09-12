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
            *(state->dst) = 0x7F; // opcode
            memcpy(state->dst + 1, state->src, 128);
            state->src += 128;
            state->dst += 1 + 128;
        }
        if(part_block_cnt) {
            *(state->dst) = (uint8_t)part_block_cnt - 1; // opcode
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
            *(state->dst) = 0xBF; // opcode
            ++(state->dst);
        }
        if(part_block_cnt) {
            *(state->dst) = (uint8_t)part_block_cnt - 0x81; // subtract -1, subtract extra 0x80 to get 1 in higher bit
            ++(state->dst);
        }
    }
}

static void ECL_ZeroEater_DumpSeq3(ECL_ZeroEaterComprssorState* state, ECL_usize count_x, ECL_usize count_0) {
    // count_x = [1..8]; count_0 = [1..8];
    ECL_usize length = count_x + 1;
    state->result_size += length;
    if((state->dst + length) <= state->dst_end) {
        *(state->dst) = ((uint8_t)count_0 - (uint8_t)0x41) | (((uint8_t)count_x - (uint8_t)1) << 3); // subtract extra 0x40 to get 11 in higher bits
        memcpy(state->dst + 1, state->src, count_x);
        state->src += count_x;
        state->dst += 1 + count_x;
    }
}

static void ECL_ZeroEater_DumpGeneric(ECL_ZeroEaterComprssorState* state, ECL_usize count_x, ECL_usize count_0) {
    // count_x > 0; count_0 > 0
    ECL_usize x_full_blocks, tmp;
    x_full_blocks = count_x >> 7;
    if(x_full_blocks) {
        tmp = count_x & 0x7F; // last block size
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
    const uint8_t* src_end = src + src_size;
    if(! src) {
        return 0;
    }
    state.src = src;
    state.result_size = 0;
    state.dst = dst;
    state.dst_end = dst ? (dst + dst_size) : dst;
    first_x = src;
    while(state.src < src_end) {
        for(first_0 = first_x; (first_0 < src_end) && *first_0; ++first_0); // search for zero from where last search ended

        cnt_x = first_0 - state.src; // count of non-zeroes in beginning
        if(first_0 == src_end) { // complete it (1)
            ECL_ZeroEater_DumpSeq1(&state, cnt_x);
            break;
        }
        // we found zero, find next non-zero
        for(first_x = first_0; (first_x < src_end) && (! *first_x); ++first_x);

        cnt_0 = first_x - first_0; // count of zeroes afterwards
        // stream looks like {state.src: [xx..x] first_0: [00..0] first_x: ...}
        if(cnt_x) { // has non-zero stream too (3)
            if((cnt_0 == 1) && (cnt_x > 8)) { // bad deal
                // this particular case is the worst, this single zero-byte should be considered as non-zero for better compression
                // this data will be processed in next iteration
                continue;
            }
            ECL_ZeroEater_DumpGeneric(&state, cnt_x, cnt_0);
            state.src = first_x;
        } else { // only zero stream (2)
            ECL_ZeroEater_DumpSeq2(&state, cnt_0);
            state.src = first_x;
        }
    }
    return state.result_size;
}

ECL_usize ECL_ZeroEater_Decompress(const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size) {
    ECL_usize dst_pos, src_pos, cnt_x, cnt_0;
    uint8_t opcode;
    if((! src) || (! dst)) { // invalid parameters
        return 0;
    }
    dst_pos = 0;
    for(src_pos = 0; src_pos < src_size;) {
        opcode = src[src_pos];
        ++src_pos;
        cnt_x = 0;
        cnt_0 = 0;
        if(opcode & 0x80) { // method 2 or 3
            if(opcode & 0x40) { // method 3
                cnt_x = ((opcode >> 3) & 0x07) + 1;
                cnt_0 = (opcode & 0x07) + 1;
            } else { // method 2
                cnt_0 = (opcode & 0x3F) + 1;
            }
        } else { // method 1
            cnt_x = (opcode & 0x7F) + 1;
        }
        // check if fits output buffer and unpack
        if((dst_pos + cnt_x + cnt_0) > dst_size) {
            break;
        }
        if(cnt_x) {
            if((src_pos + cnt_x) > src_size) {
                break; // invalid stream
            }
            memcpy(dst + dst_pos, src + src_pos, cnt_x);
            dst_pos += cnt_x;
        }
        if(cnt_0) {
            memset(dst + dst_pos, 0, cnt_0);
            dst_pos += cnt_0;
        }
        src_pos += cnt_x;
    }
    return (src_pos == src_size) ? dst_pos : 0; // ensure all source is consumed
}
