/*
 * Copyright 2025 - 2025 Evgeniy Evstratov
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

#include "ECL_Huff8.h"
#include "ECL_JH_States.h"
#include "ECL_utils.h"

// sorts uint16_t values in order of descendance, sorts 'codes' array affecting it's order equally to 'values' array (swaps occur at same indices)
void ECL_Huff8_Aux_QSort16(uint8_t* codes, uint16_t* values, int size, uint8_t* buf512) {
    /* technically less buf is suffinicient, but it's hard to calculate */
    uint8_t* stack_first = buf512;
    uint8_t* stack_nth = buf512 + 256;
    uint16_t stack_depth;
    //
    stack_first[0] = 0;
    stack_nth[0] = (uint8_t)(size - 1);
    stack_depth = 1;
    //
    while(stack_depth) {
        --stack_depth;
        int first = stack_first[stack_depth];
        int nth = stack_nth[stack_depth];
        int dist = (nth - first);
        //
        if(dist < 16) { /* limit here affects used stack (buf512) depth */
            int i, j;
            uint8_t db;
            uint16_t dw;
            for(i = first + 1; i <= nth; ++i) {
                db = codes[i];
                dw = values[i];
                for(j = i - 1; (j >= first) && values[j] < dw; --j) {
                    codes[j+1] = codes[j];
                    values[j+1] = values[j];
                }
                values[j+1] = dw;
                codes[j+1] = db;
            }
        } else {
            /* nth must be >= first */
            const uint16_t candidate = values[first + (dist/2)];
            int i = first, j = nth;
            while(i < j) {
                while(values[i] > candidate) {
                    ++i;
                }
                while(values[j] < candidate) {
                    --j;
                }
                if(i >= j) {
                    break;
                }
                uint8_t db = codes[i];
                codes[i] = codes[j];
                codes[j] = db;
                uint16_t dw = values[i];
                values[i] = values[j];
                values[j] = dw;
                ++i;
                --j;
            }
            if(j > first) {
                stack_first[stack_depth] = first;
                stack_nth[stack_depth] = j;
                ++stack_depth;
            }
            if(i < nth) {
                stack_first[stack_depth] = i;
                stack_nth[stack_depth] = nth;
                ++stack_depth;
            }
        }
    }
}

int16_t ECL_Huff8_Freqs16ToTSpec1024_ULM(uint16_t* freqs/*[256]*/, uint32_t* out_tspec1024/*[256]*/, uint8_t* buf256, uint8_t* out_ctree768, uint16_t n_unique_max) {
    int16_t n_unique = 0; // amount of unique values
    //
    ECL_ASSERT(out_tspec1024 == ECL_GetAlignedPointer4((uint8_t*)out_tspec1024));
    ECL_ASSERT(freqs == ECL_GetAlignedPointer2((uint8_t*)freqs));
    //
    for(int16_t i = 0; i < 256; ++i) {
        if(freqs[i]) {
            buf256[n_unique] = i;
            freqs[n_unique] = freqs[i];
            ++n_unique;
        }
    }
    if(n_unique_max && (n_unique > n_unique_max)) { // reserved API extension
        n_unique = n_unique_max;
    }
    if(n_unique < 2) {
        return n_unique;
    }
    ECL_Huff8_Aux_QSort16(buf256, freqs, n_unique, out_ctree768);
    { // form tree
        ECL_SCOPED_CONST int16_t n_unique_m1 = n_unique - 1;
        int16_t next_leaf;
        int16_t branches_top;
        int16_t next_branch;

        next_leaf = n_unique_m1;
        branches_top = n_unique_m1;
        next_branch = branches_top;
        // now form branch nodes and add backwards since the end overriding original freqs
        while(branches_top) { // process nodes till branches top reaches limit
            int16_t idleft;
            int16_t idright;
            int16_t tree_record_pos;
            if(next_leaf >= 0) {
                if(next_branch > branches_top) { // both halves non-empty
                    if(freqs[next_leaf] < freqs[next_branch]) {
                        idleft = next_leaf; // this is the least value now
                        if(next_leaf && (freqs[next_leaf - 1] < freqs[next_branch])) {
                            idright = next_leaf - 1;
                            --next_leaf;
                        } else {
                            idright = next_branch;
                            --next_branch;
                        }
                        --next_leaf;
                    } else { // next leave has greater value than next branch
                        idleft = next_branch; // this is the least value now
                        --next_branch;
                        if((next_branch > branches_top) && (freqs[next_leaf] >= freqs[next_branch])) {
                            idright = next_branch;
                            --next_branch;
                        } else {
                            idright = next_leaf;
                            --next_leaf;
                        }
                    }
                } else { // only first half non-empty (and has at least two nodes)
                    idleft = next_leaf;
                    idright = next_leaf - 1;
                    next_leaf -= 2;
                }
            } else { // only second half is non-empty (and has at least two nodes)
                idleft = next_branch;
                idright = next_branch - 1;
                next_branch -= 2;
            }
            // form tree - recalculate virtual indices of branches according to freqs placement
            tree_record_pos = n_unique_m1 - branches_top;
            if(idleft <= branches_top) { // leaf/value
                out_ctree768[tree_record_pos*2 + 0] = buf256[idleft];
                out_ctree768[tree_record_pos + 512] = 1;
            } else {
                out_ctree768[tree_record_pos*2 + 0] = n_unique_m1 - idleft;
                out_ctree768[tree_record_pos + 512] = 0;
            }
            if(idright <= branches_top) { // leaf/value
                out_ctree768[tree_record_pos*2 + 1] = buf256[idright];
                out_ctree768[tree_record_pos + 512] |= 2;
            } else {
                out_ctree768[tree_record_pos*2 + 1] = n_unique_m1 - idright;
            }
            //
            freqs[branches_top] = freqs[idleft] + freqs[idright];
            --branches_top;
        }
        ECL_ASSERT(branches_top == 0);
    }
    // root node index in tree == freqs_top - 1 - n_unique == n_unique - 2;
    { // form spec
        uint8_t* ECL_SCOPED_CONST stack_flags = buf256; // 0: entered left; 1: entered right; allocate arrays within free buffer
        uint8_t* ECL_SCOPED_CONST stack_ptrs = buf256 + ECL_HUFF8_TREE_DEPTH_MAX_ULM; // -:- same size
        uint16_t tree_record_pos = n_unique - 2; // root
        uint16_t stack_depth = 0;
        uint32_t tmp_code = 0; // could be smaller for smaller datasets:
        // 16 bits to guarantee work for any dataset of < 4180 bytes
        // 24 bits ~= 196k bytes
        uint8_t code_len = 0;
        uint8_t checking_side = 0; // 0/1 (left/right)
        // pre-clear spec
        memset(out_tspec1024, 0, 1024);
        do {
            ECL_SCOPED_CONST uint8_t side_value = out_ctree768[tree_record_pos*2 + checking_side];
            if(checking_side) { // go right
                tmp_code >>= 1; tmp_code |= 0x80000000; // ensure leading 1
            } else { // go left
                tmp_code >>= 1; tmp_code &= 0x7FFFFFFF; // ensure leading 0
            }
            ++code_len;
            //
            if(out_ctree768[tree_record_pos + 512] & (checking_side ? 2 : 1)) { // checked side ('left' or 'right') is leaf/value
                //
                ECL_ASSERT(out_tspec1024[side_value] == 0);
                out_tspec1024[side_value] = ((tmp_code >> (32 - code_len)) & 0x00FFFFFF) | (((uint32_t)code_len) << 24);
                //
                if(checking_side) { // checked right - return back thru stack
                    uint8_t quit = 1;
                    while(stack_depth) {
                        --stack_depth;
                        tree_record_pos = stack_ptrs[stack_depth];
                        tmp_code <<= 1; // erase leading bit
                        --code_len;
                        if(stack_flags[stack_depth] == 0) { // 'left' was entered there - return to that record
                            // checking_side = 1; // already 1 in this codepath
                            tmp_code <<= 1; // erase leading bit
                            --code_len;
                            quit = 0;
                            break;
                        } // else - 'right' was entered there - keep returning deeper
                    }
                    if(quit) {
                        break;
                    }
                } else { // else - go to checking 'right'
                    checking_side = 1;
                    tmp_code <<= 1; // erase leading bit
                    --code_len;
                }
            } else {
                ECL_ASSERT(stack_depth < ECL_HUFF8_TREE_DEPTH_MAX_ULM);
                stack_flags[stack_depth] = checking_side;
                stack_ptrs[stack_depth] = tree_record_pos;
                ++stack_depth;
                tree_record_pos = side_value;
                checking_side = 0;
            }
        } while(1);
    }
    return n_unique;
}


/* Evaluate/Analyze user methods - not modifying, estimate how much space is needed for compressed output ------------------------------------------ */

uint32_t ECL_Huff8_EvaluateTree(uint16_t n_unique) {
    return ECL_HUFF8_COMPRESSED_TREE_SIZE_BITS(n_unique);
}

uint32_t ECL_Huff8_Analyze16_ULM(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint32_t* buf1024, uint8_t* buf256, uint8_t* buf768) {
    uint16_t* ECL_SCOPED_CONST freqs_buf = (uint16_t*)buf1024;
    int16_t n_unique;
    uint32_t result_bits;

    memset(freqs_buf, 0, 512);
    for(uint32_t i = 0, ofs = 0; i < bytes_cnt; ++i, ofs += interval) {
        ++freqs_buf[src[ofs]];
    }
    n_unique = ECL_Huff8_Freqs16ToTSpec1024_ULM(freqs_buf, buf1024, buf256, buf768, 0);

    if(n_unique < 2) {
        return 9; // 9 bits
    }
    result_bits = (n_unique * 10) - 1;
    for(uint32_t i = 0, ofs = 0; i < bytes_cnt; ++i, ofs += interval) {
        ECL_SCOPED_CONST uint32_t n_bits = (buf1024[src[ofs]] >> 24) & 0x0FF;
        result_bits += n_bits;
    }
    return result_bits;
}

uint32_t ECL_Huff8_Analyze16_ULM_2k5(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint32_t* buf1024, uint8_t* buf256, uint8_t* buf768, uint16_t* buf512) {
    uint16_t* ECL_SCOPED_CONST freqs_buf = (uint16_t*)buf1024;
    int16_t n_unique;
    uint32_t result_bits;

    memset(freqs_buf, 0, 512);
    for(uint32_t i = 0, ofs = 0; i < bytes_cnt; ++i, ofs += interval) {
        ++freqs_buf[src[ofs]];
    }
    memcpy(buf512, freqs_buf, 512);
    n_unique = ECL_Huff8_Freqs16ToTSpec1024_ULM(freqs_buf, buf1024, buf256, buf768, 0);

    if(n_unique < 2) {
        return 9; // 9 bits
    }
    result_bits = (n_unique * 10) - 1;
    for(uint16_t i = 0; i < 256; ++i) {
        if(buf512[i]) {
            ECL_SCOPED_CONST uint32_t n_bits = (buf1024[i] >> 24) & 0x0FF;
            result_bits += (buf512[i] * n_bits);
        }
    }
    return result_bits;
}



/* Compress* user methods -------------------------------------------------------------------------------------------------------------------------- */

void ECL_Huff8_CompressCTree768(const uint8_t* ctree768, uint16_t n_unique, uint8_t* depth_buf_x2, uint16_t depth_buf_size, ECL_HUFF8_WSTREAM_Type* wstream) {
    uint8_t* ECL_SCOPED_CONST stack_flags = depth_buf_x2; // 0: entered left; 1: entered right; allocate arrays within free buffer
    uint8_t* ECL_SCOPED_CONST stack_ptrs = depth_buf_x2 + depth_buf_size; // -:- same size
    uint16_t tree_record_pos = n_unique - 2; // root
    uint16_t stack_depth = 0;
    uint8_t checking_side = 0; // 0/1 (left/right)

    ECL_ASSERT(n_unique > 1);
    ECL_HUFF8_WSTREAM_Write1_8(wstream, 0, 1);
    do {
        ECL_SCOPED_CONST uint8_t side_value = ctree768[tree_record_pos*2 + checking_side];
        //
        if(ctree768[tree_record_pos + 512] & (checking_side ? 2 : 1)) { // checked side ('left' or 'right') is leaf/value
            //
            ECL_HUFF8_WSTREAM_Write1_8(wstream, 1, 1);
            ECL_HUFF8_WSTREAM_Write1_8(wstream, side_value, 8);
            //
            if(checking_side) { // checked right - return back thru stack
                uint8_t quit = 1;
                while(stack_depth) {
                    --stack_depth;
                    tree_record_pos = stack_ptrs[stack_depth];
                    if(stack_flags[stack_depth] == 0) { // 'left' was entered there - return to that record
                        // checking_side = 1; // already 1 in this codepath
                        quit = 0;
                        break;
                    } // else - 'right' was entered there - keep returning deeper
                }
                if(quit) {
                    break;
                }
            } else { // else - go to checking 'right'
                checking_side = 1;
            }
        } else {
            ECL_HUFF8_WSTREAM_Write1_8(wstream, 0, 1);
            //
            ECL_ASSERT(stack_depth < depth_buf_size);
            stack_flags[stack_depth] = checking_side;
            stack_ptrs[stack_depth] = tree_record_pos;
            ++stack_depth;
            tree_record_pos = side_value;
            checking_side = 0;
        }
    } while(1);
}

/* compresses only data itself, returns amount of bits written (or 0 in case of error) */
uint32_t ECL_Huff8_CompressDataWithTSpec1024(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, const uint32_t* tspec1024/*[256]*/, ECL_HUFF8_WSTREAM_Type* wstream) {
    uint32_t result_bits = 0;
    ECL_ASSERT(tspec1024 == ECL_GetAlignedConstPointer4((const uint8_t*)tspec1024));
    //
    for(uint32_t i = 0, ofs = 0; i < bytes_cnt; ++i, ofs += interval) {
        uint32_t info = tspec1024[src[ofs]];
        uint8_t n_bits = (uint8_t)(info >> 24);
        result_bits += n_bits;
        while(n_bits > 8) {
            ECL_HUFF8_WSTREAM_Write1_8(wstream, (uint8_t)info, 8);
            n_bits -= 8;
            info >>= 8;
        }
        ECL_HUFF8_WSTREAM_Write1_8(wstream, (uint8_t)info, n_bits);
    }
    return result_bits;
}

uint32_t ECL_Huff8_Compress16_ULM(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint32_t* buf1024/*[256]*/, uint8_t* buf256, uint8_t* buf768, ECL_HUFF8_WSTREAM_Type* wstream) {
    uint16_t* ECL_SCOPED_CONST freqs_buf = (uint16_t*)buf1024;
    int16_t n_unique;
    uint32_t result_bits;
    ECL_ASSERT(buf1024 == ECL_GetAlignedPointer4((uint8_t*)buf1024));

    memset(freqs_buf, 0, 512);
    for(uint32_t i = 0, ofs = 0; i < bytes_cnt; ++i, ofs += interval) {
        ++freqs_buf[src[ofs]];
    }
    n_unique = ECL_Huff8_Freqs16ToTSpec1024_ULM(freqs_buf, buf1024, buf256, buf768, 0);
    if(n_unique < 2) {
        if(n_unique == 1) {
            ECL_HUFF8_WSTREAM_Write1_8(wstream, 1, 1);
            ECL_HUFF8_WSTREAM_Write1_8(wstream, src[0], 8);
        }
        return 9; // 9 bits
    }
    result_bits = ECL_Huff8_EvaluateTree(n_unique);
    // write data
    ECL_Huff8_CompressCTree768(buf768, n_unique, buf256, 128, wstream);
    result_bits += ECL_Huff8_CompressDataWithTSpec1024(src, bytes_cnt, interval, buf1024, wstream);
    return result_bits;
}

#ifdef ECL_HUFF8_WSTREAM_Init
uint32_t ECL_Huff8_Compress16_ULM_Raw(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint32_t* buf1024/*[256]*/, uint8_t* buf256, uint8_t* buf768, uint8_t* dst, ECL_usize dst_size) {
    ECL_HUFF8_WSTREAM_Type wstream;
    ECL_HUFF8_WSTREAM_Init(&wstream, dst, dst_size);
    return ECL_Huff8_Compress16_ULM(src, bytes_cnt, interval, buf1024, buf256, buf768, &wstream);
}
#endif



/* Decompress* user methods ------------------------------------------------------------------------------------------------------------------------ */

void ECL_Huff8_DecompressDTree1025(ECL_HUFF8_RSTREAM_Type* rstream, uint8_t* dst_dtree1025) {
    // dtree node is 16 bit. either 0x80_value_code or 0x0_right_node_index (0x0_right_node_index takes 9 bits)
    uint16_t* dtree_buf = ECL_GetAlignedPointer2(dst_dtree1025);
    uint16_t unp_stack[ECL_HUFF8_DECOMPRESS_MAX_DEPTH];
    uint16_t next_node = 1;
    uint16_t stack_depth = 0;
    uint16_t curr_node = 0;
    //
    while(1) {
        if(ECL_HUFF8_RSTREAM_Read1_8(rstream, 1)) { // leaf
            uint16_t code = (uint16_t)ECL_HUFF8_RSTREAM_Read1_8(rstream, 8);
            dtree_buf[curr_node] = 0x8000 | code;
            while(stack_depth) { // return to parent and check it's state
                --stack_depth;
                curr_node = unp_stack[stack_depth];
                if(! dtree_buf[curr_node]) { // left side done, go right
                    ++stack_depth;
                    if(next_node >= 511) {
                        return; // ERROR: invalid data (tree branches more than for 256 codes)
                    }
                    dtree_buf[curr_node] = next_node; // ref right node
                    curr_node = next_node; // allocate right node
                    ++next_node;
                    break;
                } // else - right side done - move up
            }
            if(! curr_node) {
                break; // done, tree is unpacked
            }
        } else { // branching
            if(stack_depth >= ECL_HUFF8_DECOMPRESS_MAX_DEPTH) {
                return; // ERROR, either invalid data or not supported tree depth
            }
            if(next_node >= 511) {
                return; // ERROR: invalid data (tree branches more than for 256 codes)
            }
            unp_stack[stack_depth] = curr_node;
            ++stack_depth;
            dtree_buf[curr_node] = 0; // '0' is a state before entering 'left' part (possible only during tree unpacking)
            curr_node = next_node;
            ++next_node;
            // go left
        }
    } // tree unpacked
    dtree_buf[next_node] = 0; // such thing will allow easy finding the end of dtree_buf for an algorithm accepting tree as parameter
    // TODO_BEFORE_HUFF8_RELEASE return size of the dtree in appropriate form
}

ECL_usize ECL_Huff8_DecompressWithDTree1025(const uint8_t* dtree1025, ECL_HUFF8_RSTREAM_Type* rstream, uint8_t* dst, ECL_usize bytes_cnt, ECL_usize interval) {
    if(! bytes_cnt) {
        ECL_ASSERT(false && "Compressed Huffman block can't be zero-length!");
        return 0;
    }
    const uint16_t* dtree_buf = ECL_GetAlignedConstPointer2(dtree1025);
    if(! dtree_buf[0]) {
        return 0; // error: no tree
    }
    if(! dtree_buf[1]) {
        uint8_t code = (uint8_t)dtree_buf[0];
        // only single code in tree - fill data with it
        for(ECL_usize i = 0, ofs = 0; i < bytes_cnt; ++i, ofs += interval) {
            dst[ofs] = code;
        }
        return bytes_cnt; // ok
    }
    // else - process as regular tree
#ifdef ECL_HUFF8_RSTREAM_Peek
    {
        uint8_t nbits, last_nbits;
        ECL_HUFF8_RSTREAM_Peek_ResultType val;
        val = ECL_HUFF8_RSTREAM_Peek(rstream, &nbits);
        last_nbits = nbits;
        //
        for(ECL_usize last_ofs = (interval * bytes_cnt), ofs = 0; ofs < last_ofs; ofs += interval) {
            uint16_t curr_node = 0; // we're sure it's not a leaf (checked that case above)
            do {
                if(! nbits) {
#ifdef ECL_HUFF8_RSTREAM_Peek_Multibyte
                    ECL_HUFF8_RSTREAM_Advance(rstream, last_nbits); /* consume processed data, advance rstream */
                    /* peek next */
                    val = ECL_HUFF8_RSTREAM_Peek(rstream, &nbits);
                    last_nbits = nbits;
#else /* --------------------------- */
                    ECL_HUFF8_RSTREAM_Read1_8(rstream, last_nbits);
                    /* peek next */
                    val = ECL_HUFF8_RSTREAM_Peek(rstream, &nbits);
                    last_nbits = nbits;
#endif /* ECL_HUFF8_RSTREAM_Peek_Multibyte */
                }
                if(val & 1) { // go right
                    curr_node = dtree_buf[curr_node];
                } else { /* go left (always next after parent) */
                    ++curr_node;
                }
                val >>= 1;
                --nbits;
            } while(! (dtree_buf[curr_node] & 0x8000));
            dst[ofs] = (uint8_t)dtree_buf[curr_node];
        }
        /* done */
        nbits = last_nbits - nbits;
        /* consume processed data, advance rstream */
#ifdef ECL_HUFF8_RSTREAM_Peek_Multibyte
        ECL_HUFF8_RSTREAM_Advance(rstream, nbits);
#else /* --------------------------- */
        ECL_HUFF8_RSTREAM_Read1_8(rstream, nbits);
#endif /* ECL_HUFF8_RSTREAM_Peek_Multibyte */
    }
#else
    for(ECL_usize last_ofs = (interval * bytes_cnt), ofs = 0; ofs < last_ofs; ofs += interval) {
        uint16_t curr_node = 0; // we're sure it's not a leaf (checked that case above)
        do {
            if(ECL_HUFF8_RSTREAM_Read1_8(rstream, 1)) { // go right
                curr_node = dtree_buf[curr_node];
            } else { // go left (always next after parent)
                ++curr_node;
            }
        } while(! (dtree_buf[curr_node] & 0x8000));
        dst[ofs] = (uint8_t)dtree_buf[curr_node];
    }
#endif
    return bytes_cnt; // ok if stream isn't invalidated
}

ECL_usize ECL_Huff8_Decompress(ECL_HUFF8_RSTREAM_Type* rstream, uint8_t* dtree_buf/*1025*/, uint8_t* dst, ECL_usize bytes_cnt, ECL_usize interval) {
    ECL_Huff8_DecompressDTree1025(rstream, dtree_buf);
    return ECL_Huff8_DecompressWithDTree1025(dtree_buf, rstream, dst, bytes_cnt, interval);
}

#ifdef ECL_HUFF8_RSTREAM_Init
ECL_usize ECL_Huff8_Decompress_Raw(const uint8_t* src, ECL_usize src_size, uint8_t* dtree_buf/*1025*/, uint8_t* dst, ECL_usize bytes_cnt, ECL_usize interval) {
    ECL_HUFF8_RSTREAM_Type rstream;
    ECL_HUFF8_RSTREAM_Init(&rstream, src, src_size);

    ECL_Huff8_DecompressDTree1025(&rstream, dtree_buf);
    if(! ECL_Huff8_DecompressWithDTree1025(dtree_buf, &rstream, dst, bytes_cnt, interval)) {
        return 0;
    }
    if(! rstream.is_valid) {
        return 0;
    }
    return rstream.next - src;
}
#endif
