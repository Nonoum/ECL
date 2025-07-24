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
static void ECL_Huff8_Aux_Sort16(uint8_t* codes, uint16_t* values, int size) {
    if(size < 32) {
        int i, j;
        uint8_t db;
        uint16_t dw;
        for(i = 1; i < size; ++i) {
            db = codes[i];
            dw = values[i];
            for(j = i - 1; (j >= 0) && values[j] < dw; --j) {
                codes[j+1] = codes[j];
                values[j+1] = values[j];
            }
            values[j+1] = dw;
            codes[j+1] = db;
        }
        return;
    }
    const uint16_t candidate = values[size >> 1];
    int i = 0, j = size - 1;
    while(i < j)
    {
        while(values[i] > candidate) {
            ++i;
        }
        while(values[j] < candidate) {
            --j;
        }
        if(i > j) {
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
    // TODO replace recursion with array allocated on externally supplied buffer
    if(j > 0) {
        ECL_Huff8_Aux_Sort16(codes, values, j + 1);
    }
    if(i < size) {
        ECL_Huff8_Aux_Sort16(codes + i, values + i, size - i);
    }
}

// returns amount of unique values (where freqs[i] are non-zero).
int16_t ECL_Huff8_Freqs16ToSpec1024_ULM(uint16_t* freqs/*[512]*/, uint32_t* out_spec1024/*[256]*/, uint8_t* buf256, uint8_t* out_tree768, uint16_t n_unique_max) {
    int16_t freqs_top = 0;
    int16_t n_unique; // amount of unique values
    //
    ECL_ASSERT(out_spec1024 == ECL_GetAlignedPointer4((uint8_t*)out_spec1024));
    ECL_ASSERT(freqs == ECL_GetAlignedPointer2((uint8_t*)freqs));
    //
    for(int16_t i = 0; i < 256; ++i) {
        if(freqs[i]) {
            buf256[freqs_top] = i;
            freqs[freqs_top] = freqs[i];
            ++freqs_top;
        }
    }
    if(n_unique_max && (freqs_top > n_unique_max)) { // reserved API extension
        freqs_top = n_unique_max;
    }
    n_unique = freqs_top;
    if(n_unique < 2) {
        return n_unique;
    }
    ECL_Huff8_Aux_Sort16(buf256, freqs, freqs_top);
    { // form tree
        int16_t next_leaf;
        int16_t next_branch;
        int16_t unprocessed_cnt; // amount of unprocessed logical nodes

        next_leaf = freqs_top - 1;
        next_branch = freqs_top;
        unprocessed_cnt = freqs_top;
        // now form branch nodes and add after 'freqs_top' (in order of ascendance)
        while(unprocessed_cnt >= 2) { // while have raw nodes - create a new parent node
            int16_t idleft;
            int16_t idright;
            int16_t tree_record_pos;
            if(next_leaf >= 0) {
                if(next_branch < freqs_top) { // both halves non-empty
                    if(freqs[next_leaf] < freqs[next_branch]) {
                        idleft = next_leaf; // this is the least value now
                        if(next_leaf && (freqs[next_leaf - 1] < freqs[next_branch])) {
                            idright = next_leaf - 1;
                            --next_leaf;
                        } else {
                            idright = next_branch;
                            ++next_branch;
                        }
                        --next_leaf;
                    } else { // next leave has greater value than next branch
                        idleft = next_branch; // this is the least value now
                        ++next_branch;
                        if((next_branch < freqs_top) && (freqs[next_leaf] >= freqs[next_branch])) {
                            idright = next_branch;
                            ++next_branch;
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
                idright = next_branch + 1;
                next_branch += 2;
            }
            // form tree
            tree_record_pos = freqs_top - n_unique;
            if(idleft < n_unique) { // leaf/value
                out_tree768[tree_record_pos*2 + 0] = buf256[idleft];
                out_tree768[tree_record_pos + 512] = 1;
            } else {
                out_tree768[tree_record_pos*2 + 0] = idleft - n_unique;
                out_tree768[tree_record_pos + 512] = 0;
            }
            if(idright < n_unique) { // leaf/value
                out_tree768[tree_record_pos*2 + 1] = buf256[idright];
                out_tree768[tree_record_pos + 512] |= 2;
            } else {
                out_tree768[tree_record_pos*2 + 1] = idright - n_unique;
            }
            //
            freqs[freqs_top] = freqs[idleft] + freqs[idright];
            --unprocessed_cnt;
            ++freqs_top;
        }
    }
    ECL_ASSERT(freqs_top == (n_unique*2 - 1));
    // root node index in tree == freqs_top - 1 - n_unique == n_unique*2 - 2;
    { // form spec
#define ECL_HUFF8_ULM_TREE_DEPTH_MAX 24
        uint8_t* ECL_SCOPED_CONST stack_flags = buf256; // 0: entered left; 1: entered right; allocate arrays within free buffer
        uint8_t* ECL_SCOPED_CONST stack_ptrs = buf256 + ECL_HUFF8_ULM_TREE_DEPTH_MAX; // -:- same size
        uint16_t tree_record_pos = freqs_top - 1 - n_unique; // root
        uint16_t stack_depth = 0;
        uint32_t tmp_code = 0; // could be smaller for smaller datasets:
        // 16 bits to guarantee work for any dataset of < 4180 bytes
        // 24 bits ~= 196k bytes
        uint8_t code_len = 0;
        uint8_t checking_side = 0; // 0/1 (left/right)
        // pre-clear spec
        memset(out_spec1024, 0, 1024);
        do {
            ECL_SCOPED_CONST uint8_t side_value = out_tree768[tree_record_pos*2 + checking_side];
            if(checking_side) { // go right
                tmp_code >>= 1; tmp_code |= 0x80000000; // ensure leading 1
            } else { // go left
                tmp_code >>= 1; tmp_code &= 0x7FFFFFFF; // ensure leading 0
            }
            ++code_len;
            //
            if(out_tree768[tree_record_pos + 512] & (checking_side ? 2 : 1)) { // checked side ('left' or 'right') is leaf/value
                //
                ECL_ASSERT(out_spec1024[side_value] == 0);
                out_spec1024[side_value] = ((tmp_code >> (32 - code_len)) & 0x00FFFFFF) | (((uint32_t)code_len) << 24);
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
                ECL_ASSERT(stack_depth < ECL_HUFF8_ULM_TREE_DEPTH_MAX);
                stack_flags[stack_depth] = checking_side;
                stack_ptrs[stack_depth] = tree_record_pos;
                ++stack_depth;
                tree_record_pos = side_value;
                checking_side = 0;
            }
        } while(1);
#undef ECL_HUFF8_ULM_TREE_DEPTH_MAX
    }
    return n_unique;
}


/* Evaluate/Analyze user methods - not modifying, estimate how much space is needed for compressed output ------------------------------------------ */

uint32_t ECL_Huff8_EvaluateTree(uint16_t n_unique) {
    return (n_unique * 10) - 1;
}

uint32_t ECL_Huff8_Analyze16_ULM(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint32_t* buf1024, uint8_t* buf256, uint8_t* buf768) {
    uint16_t* ECL_SCOPED_CONST freqs_buf = (uint16_t*)buf1024;
    int16_t n_unique;
    uint32_t result_bits;

    memset(freqs_buf, 0, 512);
    for(uint32_t i = 0, ofs = 0; i < bytes_cnt; ++i, ofs += interval) {
        ++freqs_buf[src[ofs]];
    }
    n_unique = ECL_Huff8_Freqs16ToSpec1024_ULM(freqs_buf, buf1024, buf256, buf768, 0);

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
    n_unique = ECL_Huff8_Freqs16ToSpec1024_ULM(freqs_buf, buf1024, buf256, buf768, 0);

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

void ECL_Huff8_CompressTree768(const uint8_t* tree768, uint16_t n_unique, uint8_t* buf48, ECL_HUFF8_WSTREAM_Type* wstream) {
#define ECL_HUFF8_ULM_TREE_DEPTH_MAX 24 // TODO hardcoded here
    uint8_t* ECL_SCOPED_CONST stack_flags = buf48; // 0: entered left; 1: entered right; allocate arrays within free buffer
    uint8_t* ECL_SCOPED_CONST stack_ptrs = buf48 + ECL_HUFF8_ULM_TREE_DEPTH_MAX; // -:- same size
    uint16_t tree_record_pos = n_unique - 2; // root
    uint16_t stack_depth = 0;
    uint8_t checking_side = 0; // 0/1 (left/right)

    ECL_ASSERT(n_unique > 1);
    ECL_HUFF8_WSTREAM_Write1_8(wstream, 0, 1);
    do {
        ECL_SCOPED_CONST uint8_t side_value = tree768[tree_record_pos*2 + checking_side];
        //
        if(tree768[tree_record_pos + 512] & (checking_side ? 2 : 1)) { // checked side ('left' or 'right') is leaf/value
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
            ECL_ASSERT(stack_depth < ECL_HUFF8_ULM_TREE_DEPTH_MAX);
            stack_flags[stack_depth] = checking_side;
            stack_ptrs[stack_depth] = tree_record_pos;
            ++stack_depth;
            tree_record_pos = side_value;
            checking_side = 0;
        }
    } while(1);
#undef ECL_HUFF8_ULM_TREE_DEPTH_MAX
}

/* compresses only data itself, returns amount of bits written (or 0 in case of error) */
uint32_t ECL_Huff8_CompressDataWithSpec1024(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, const uint32_t* spec1024/*[256]*/, ECL_HUFF8_WSTREAM_Type* wstream) {
    uint32_t result_bits = 0;
    ECL_ASSERT(spec1024 == ECL_GetAlignedConstPointer4((const uint8_t*)spec1024));
    //
    for(uint32_t i = 0, ofs = 0; i < bytes_cnt; ++i, ofs += interval) {
        uint32_t info = spec1024[src[ofs]];
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
    n_unique = ECL_Huff8_Freqs16ToSpec1024_ULM(freqs_buf, buf1024, buf256, buf768, 0);
    if(n_unique < 2) {
        if(n_unique == 1) {
            ECL_HUFF8_WSTREAM_Write1_8(wstream, 1, 1);
            ECL_HUFF8_WSTREAM_Write1_8(wstream, src[0], 8);
        }
        return 9; // 9 bits
    }
    result_bits = ECL_Huff8_EvaluateTree(n_unique);
    // write data
    ECL_Huff8_CompressTree768(buf768, n_unique, buf256, wstream);
    result_bits += ECL_Huff8_CompressDataWithSpec1024(src, bytes_cnt, interval, buf1024, wstream);
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

void ECL_Huff8_DecompressTree1025(ECL_HUFF8_RSTREAM_Type* rstream, uint8_t* dst_dtree1025) {
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
    // TODO return size of the dtree in appropriate form
}

ECL_usize ECL_Huff8_DecompressWithTree1025(const uint8_t* dtree1025, ECL_HUFF8_RSTREAM_Type* rstream, uint8_t* dst, ECL_usize bytes_cnt, ECL_usize interval) {
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
    // TODO could be optimized with some 'peek byte' function
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
    return bytes_cnt; // ok if stream isn't invalidated
}

ECL_usize ECL_Huff8_Decompress(ECL_HUFF8_RSTREAM_Type* rstream, uint8_t* dtree_buf/*1025*/, uint8_t* dst, ECL_usize bytes_cnt, ECL_usize interval) {
    ECL_Huff8_DecompressTree1025(rstream, dtree_buf);
    return ECL_Huff8_DecompressWithTree1025(dtree_buf, rstream, dst, bytes_cnt, interval);
}

#ifdef ECL_HUFF8_RSTREAM_Init
ECL_usize ECL_Huff8_Decompress_Raw(const uint8_t* src, ECL_usize src_size, uint8_t* dtree_buf/*1025*/, uint8_t* dst, ECL_usize bytes_cnt, ECL_usize interval) {
    ECL_HUFF8_RSTREAM_Type rstream;
    ECL_HUFF8_RSTREAM_Init(&rstream, src, src_size);

    ECL_Huff8_DecompressTree1025(&rstream, dtree_buf);
    return ECL_Huff8_DecompressWithTree1025(dtree_buf, &rstream, dst, bytes_cnt, interval);
}
#endif
