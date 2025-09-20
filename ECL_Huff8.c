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

/* sorts uint16_t values in order of descendance, sorts 'codes' array affecting it's order equally to 'values' array (swaps occur at same indices) */
void ECL_Huff8_Aux_QSort16(uint8_t* codes, uint16_t* values, unsigned size, uint8_t* buf512) {
    /* technically less buf is suffinicient, but it's hard to calculate */
    uint8_t* stack_first = buf512;
    uint8_t* stack_nth = buf512 + 256;
    uint16_t stack_depth;
    /**/
    stack_first[0] = 0;
    stack_nth[0] = (uint8_t)(size - 1);
    stack_depth = 1;
    /**/
    while(stack_depth) {
        --stack_depth;
        int first = stack_first[stack_depth];
        int nth = stack_nth[stack_depth];
        int dist = (nth - first);
        /**/
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
            ECL_SCOPED_CONST uint16_t candidate = values[first + (dist/2)];
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
                stack_first[stack_depth] = (uint8_t)first;
                stack_nth[stack_depth] = (uint8_t)j;
                ++stack_depth;
            }
            if(i < nth) {
                stack_first[stack_depth] = (uint8_t)i;
                stack_nth[stack_depth] = (uint8_t)nth;
                ++stack_depth;
            }
        }
    }
}

/*
    Specialized sort implementation for huffman algorithm needs, works with limited data sets (maximum 'value' is < 256, 'size' is < 256).
    Allows limiting worst-case complecity possible for QSort in some cases (QSort has theoretical worst case quadratic complecity).
    Makes sense to prefer if 'size' > 20 (roughly guessed).
*/
void ECL_Huff8_Aux_HSort16(uint8_t* codes, uint16_t* values, unsigned size, uint8_t* buf768) {
    unsigned i;
    ECL_ASSERT(size < 256); /* 255 is still fine */
    /*
        buf768 is used as three bufs 256 bytes each - used with explicit offsets hoping to take advantage of [base + i*S + offs] operations
        - buf768: links to first code node (index is value of 'values')
        - buf768+256: links to next code nodes (or self, if no next)
        - buf768+512: temporary storage for sorted result of 'codes'
    */
    memset(buf768, 255, 256); /* fill with 255 (invalid as link address) */
    for(i = 0; i < size; ++i) {
        uint8_t table_pos = (uint8_t)(values[i]);
        uint8_t link = (uint8_t)i;
        if(buf768[table_pos] == 255) {
            buf768[256 + link] = link; /* first occurance of value - link to self */
        } else {
            buf768[256 + link] = buf768[table_pos]; /* already occurred earlier - link to previous */
        }
        buf768[table_pos] = link;
    }
    /* Sort in order of descendance now - write codes to temporary buffer, then copy */
    i = 0;
    for(unsigned table_pos = 255; table_pos > 0; --table_pos) { /* can go while table_pos>0, as value==0 can't occur here */
        uint8_t link = buf768[table_pos];
        if(link == 255) {
            continue;
        }
        while(1) {
            uint8_t next = buf768[256 + link];
            /* form output */
            buf768[512 + i] = codes[link];
            values[i] = (uint16_t)table_pos;
            ++i;
            if(next == link) {
                break;
            }
            link = next;
        }
    }
    memcpy(codes, buf768+512, size); /* move temp result to 'codes' */
}



void ECL_Huff8_FillFreqs16(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint16_t* freqs/*[256]*/) {
#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if((src == NULL) || (freqs == NULL)) {
        ECL_ASSERT(0);
        return;
    }
#endif
    memset(freqs, 0, 512);
    for(uint32_t i = 0, ofs = 0; i < bytes_cnt; ++i, ofs += interval) {
        ++freqs[src[ofs]];
    }
}

uint16_t ECL_Huff8_Freqs16ToCTree768(uint16_t* freqs/*[256]*/, uint8_t* buf256, uint8_t* out_ctree768, uint16_t n_unique_max) {
    uint16_t n_unique = 0; /* amount of unique values */
    /**/
#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if((freqs == NULL) || (buf256 == NULL) || (out_ctree768 == NULL)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif
    ECL_ASSERT(freqs == ECL_GetAlignedPointer2((uint8_t*)freqs));
    /**/
    for(uint16_t i = 0; i < 256; ++i) {
        if(freqs[i]) {
            buf256[n_unique] = (uint8_t)i;
            freqs[n_unique] = freqs[i];
            ++n_unique;
        }
    }
    if(n_unique_max && (n_unique > n_unique_max)) { /* reserved API extension */
        n_unique = n_unique_max;
    }
    if(n_unique < 2) {
        ECL_ASSERT(n_unique); /* if there's 0 - API is used incorrectly (had empty data stream) */
        /* save n_unique-1 (== 0..255) top last byte */
        out_ctree768[767] = 0;
        out_ctree768[766] = buf256[0]; /* save the only unique element at pre-last position for self-sufficiency */
        return n_unique;
    }
#ifndef ECL_HUFF8_DISABLE_HSORT /* HSort can be disabled to reduce binary code size */
    {
        uint8_t chose_hsort = 0;
        if((n_unique < 256) && (n_unique > 20)) {
            uint16_t i = 0;
            for(; i < n_unique; ++i) {
                if(freqs[i] >= 256) {
                    break; /* can't run HSort */
                }
            }
            if(i == n_unique) {
                chose_hsort = 1;
                ECL_Huff8_Aux_HSort16(buf256, freqs, n_unique, out_ctree768);
            }
        }
        if(! chose_hsort) {
            ECL_Huff8_Aux_QSort16(buf256, freqs, n_unique, out_ctree768);
        }
    }
#else
    ECL_Huff8_Aux_QSort16(buf256, freqs, n_unique, out_ctree768);
#endif
    { /* form tree */
        uint16_t next_leaf;
        uint16_t branches_top;
        uint16_t next_branch;

        next_leaf = n_unique - 1;
        branches_top = n_unique - 1;
        next_branch = branches_top;
        /* now form branch nodes and add backwards since the end overriding original freqs */
        while(branches_top) { /* process nodes till branches top reaches limit */
            uint16_t idleft;
            uint16_t idright;
            uint16_t tree_record_pos;
            if((int16_t)next_leaf >= 0) {
                if(next_branch > branches_top) { /* both halves non-empty */
                    if(freqs[next_leaf] <= freqs[next_branch]) {
                        idleft = next_leaf; /* this is the least value now */
                        if(next_leaf && (freqs[next_leaf - 1] <= freqs[next_branch])) { /* prioritize picking leaf to minimize worst-case depth */
                            idright = next_leaf - 1;
                            --next_leaf;
                        } else {
                            idright = next_branch;
                            --next_branch;
                        }
                        --next_leaf;
                    } else { /* next leave has greater value than next branch */
                        idleft = next_branch; /* this is the least value now */
                        --next_branch;
                        if((next_branch > branches_top) && (freqs[next_leaf] > freqs[next_branch])) { /* prioritize picking leaf to minimize worst-case depth */
                            idright = next_branch;
                            --next_branch;
                        } else {
                            idright = next_leaf;
                            --next_leaf;
                        }
                    }
                } else { /* only first half non-empty (and has at least two nodes) */
                    idleft = next_leaf;
                    idright = next_leaf - 1;
                    next_leaf -= 2;
                }
            } else { /* only second half is non-empty (and has at least two nodes) */
                idleft = next_branch;
                idright = next_branch - 1;
                next_branch -= 2;
            }
            /* form tree - recalculate virtual indices of branches according to freqs placement */
            tree_record_pos = n_unique - branches_top; /* record index/ref == 'n_unique - index in freqs', 0th index is reserved for tree root element  */
            if(idleft <= branches_top) { /* leaf/value */
                out_ctree768[tree_record_pos*2 + 0] = buf256[idleft];
                out_ctree768[tree_record_pos + 512] = 1;
            } else {
                out_ctree768[tree_record_pos*2 + 0] = (uint8_t)(n_unique - idleft);
                out_ctree768[tree_record_pos + 512] = 0;
            }
            if(idright <= branches_top) { /* leaf/value */
                out_ctree768[tree_record_pos*2 + 1] = buf256[idright];
                out_ctree768[tree_record_pos + 512] |= 2;
            } else {
                out_ctree768[tree_record_pos*2 + 1] = (uint8_t)(n_unique - idright);
            }
            /**/
            freqs[branches_top] = freqs[idleft] + freqs[idright];
            --branches_top;
        }
        ECL_ASSERT(branches_top == 0);
        /* move root element (logical index == n_unique-1) to 0th position */
        out_ctree768[0] = out_ctree768[n_unique*2 - 2];
        out_ctree768[1] = out_ctree768[n_unique*2 - 2 + 1];
        out_ctree768[512] = out_ctree768[n_unique - 1 + 512];
        /* save n_unique-1 (== 0..255) top last byte, for potential future use */
        out_ctree768[767] = (uint8_t)(n_unique - 1);
    }
    return n_unique;
}



void ECL_Huff8_CTree768ToTSpec1024_ULM(const uint8_t* ctree768, uint32_t* out_tspec1024/*[256]*/, uint8_t* depth_buf_x2) {
    uint8_t* ECL_SCOPED_CONST stack_flags = depth_buf_x2; /* 0: entered left; 1: entered right; allocate arrays within free buffer */
    uint8_t* ECL_SCOPED_CONST stack_ptrs = depth_buf_x2 + ECL_HUFF8_TREE_DEPTH_MAX_ULM; /* -:- same size */
    uint16_t tree_record_pos = 0; /* root */
    uint16_t stack_depth = 0;
    uint32_t tmp_code = 0; /* could be smaller for smaller datasets: */
    /* 16 bits to guarantee work for any dataset of <= 5776 bytes */
    /* 24 bits : <= 271441 bytes */
    uint8_t code_len = 0;
    uint8_t checking_side = 0; /* 0/1 (left/right) */

#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if((ctree768 == NULL) || (out_tspec1024 == NULL) || (depth_buf_x2 == NULL)) {
        ECL_ASSERT(0);
        return;
    }
#endif

    ECL_ASSERT(out_tspec1024 == ECL_GetAlignedPointer4((uint8_t*)out_tspec1024));
    /* pre-clear spec */
    memset(out_tspec1024, 0, 1024);
    do {
        ECL_SCOPED_CONST uint8_t side_value = ctree768[tree_record_pos*2 + checking_side];
        if(checking_side) { /* go right */
            tmp_code >>= 1; tmp_code |= 0x80000000; /* ensure leading 1 */
        } else { /* go left */
            tmp_code >>= 1; tmp_code &= 0x7FFFFFFF; /* ensure leading 0 */
        }
        ++code_len;
        /**/
        if(ctree768[tree_record_pos + 512] & (checking_side ? 2 : 1)) { /* checked side ('left' or 'right') is leaf/value */
            /**/
            ECL_ASSERT(out_tspec1024[side_value] == 0);
            out_tspec1024[side_value] = ((tmp_code >> (32 - code_len)) & 0x00FFFFFF) | (((uint32_t)code_len) << 24);
            /**/
            if(checking_side) { /* checked right - return back thru stack */
                uint8_t quit = 1;
                while(stack_depth) {
                    --stack_depth;
                    tree_record_pos = stack_ptrs[stack_depth];
                    tmp_code <<= 1; /* erase leading bit */
                    --code_len;
                    if(stack_flags[stack_depth] == 0) { /* 'left' was entered there - return to that record */
                        /* checking_side = 1; */ /* already 1 in this codepath */
                        tmp_code <<= 1; /* erase leading bit */
                        --code_len;
                        quit = 0;
                        break;
                    } /* else - 'right' was entered there - keep returning deeper */
                }
                if(quit) {
                    break;
                }
            } else { /* else - go to checking 'right' */
                checking_side = 1;
                tmp_code <<= 1; /* erase leading bit */
                --code_len;
            }
        } else {
            ECL_ASSERT(stack_depth < ECL_HUFF8_TREE_DEPTH_MAX_ULM);
            stack_flags[stack_depth] = checking_side;
            stack_ptrs[stack_depth] = (uint8_t)tree_record_pos;
            ++stack_depth;
            tree_record_pos = side_value;
            checking_side = 0;
        }
    } while(1);
}

int16_t ECL_Huff8_CTree768ToTSpec768(const uint8_t* ctree768, uint16_t* out_tspec768/*[768/2 == 384]*/, uint8_t* depth_buf_x2) {
    uint8_t* ECL_SCOPED_CONST stack_flags = depth_buf_x2; /* 0: entered left; 1: entered right; allocate arrays within free buffer */
    uint8_t* ECL_SCOPED_CONST stack_ptrs = depth_buf_x2 + ECL_HUFF8_TREE_DEPTH_MAX_TSPEC768; /* -:- same size */
    uint8_t* ECL_SCOPED_CONST nbits_table = (uint8_t*)(out_tspec768 + 256); /* tspec768 is uint16_t[256] + uint8_t[256] in series */
    uint16_t tree_record_pos = 0; /* root */
    uint16_t stack_depth = 0;
    uint16_t tmp_code = 0;
    uint8_t code_len = 0;
    uint8_t checking_side = 0; /* 0/1 (left/right) */

#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if((ctree768 == NULL) || (out_tspec768 == NULL) || (depth_buf_x2 == NULL)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif

    ECL_ASSERT(out_tspec768 == ECL_GetAlignedPointer2((uint8_t*)out_tspec768));
    /* pre-clear spec */
    memset(nbits_table, 0, 256);
    do {
        ECL_SCOPED_CONST uint8_t side_value = ctree768[tree_record_pos*2 + checking_side];
        if(checking_side) { /* go right */
            tmp_code >>= 1; tmp_code |= 0x8000; /* ensure leading 1 */
        } else { /* go left */
            tmp_code >>= 1; tmp_code &= 0x7FFF; /* ensure leading 0 */
        }
        ++code_len;
        /**/
        if(ctree768[tree_record_pos + 512] & (checking_side ? 2 : 1)) { /* checked side ('left' or 'right') is leaf/value */
            /**/
            ECL_ASSERT(nbits_table[side_value] == 0);
            out_tspec768[side_value] = (tmp_code >> (16 - code_len));
            nbits_table[side_value] = code_len;
            /**/
            if(checking_side) { /* checked right - return back thru stack */
                uint8_t quit = 1;
                while(stack_depth) {
                    --stack_depth;
                    tree_record_pos = stack_ptrs[stack_depth];
                    tmp_code <<= 1; /* erase leading bit */
                    --code_len;
                    if(stack_flags[stack_depth] == 0) { /* 'left' was entered there - return to that record */
                        /* checking_side = 1; */ /* already 1 in this codepath */
                        tmp_code <<= 1; /* erase leading bit */
                        --code_len;
                        quit = 0;
                        break;
                    } /* else - 'right' was entered there - keep returning deeper */
                }
                if(quit) {
                    break;
                }
            } else { /* else - go to checking 'right' */
                checking_side = 1;
                tmp_code <<= 1; /* erase leading bit */
                --code_len;
            }
        } else {
            if(stack_depth >= ECL_HUFF8_TREE_DEPTH_MAX_TSPEC768) {
                ECL_ASSERT(0);
                return 0;
            }
            stack_flags[stack_depth] = checking_side;
            stack_ptrs[stack_depth] = (uint8_t)tree_record_pos;
            ++stack_depth;
            tree_record_pos = side_value;
            checking_side = 0;
        }
    } while(1);
    return 1;
}

int16_t ECL_Huff8_CTree768ToTSpec512(const uint8_t* ctree768, uint16_t* out_tspec512/*[512/2 == 256]*/, uint8_t* depth_buf_x2) {
    uint8_t* ECL_SCOPED_CONST stack_flags = depth_buf_x2; /* 0: entered left; 1: entered right; allocate arrays within free buffer */
    uint8_t* ECL_SCOPED_CONST stack_ptrs = depth_buf_x2 + ECL_HUFF8_TREE_DEPTH_MAX_TSPEC512; /* -:- same size */
    uint16_t tree_record_pos = 0; /* root */
    uint16_t stack_depth = 0;
    uint16_t tmp_code = 0;
    uint8_t code_len = 0;
    uint8_t checking_side = 0; /* 0/1 (left/right) */

#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if((ctree768 == NULL) || (out_tspec512 == NULL) || (depth_buf_x2 == NULL)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif

    ECL_ASSERT(out_tspec512 == ECL_GetAlignedPointer2((uint8_t*)out_tspec512));
    /* pre-clear spec */
    memset(out_tspec512, 0, 512);
    do {
        ECL_SCOPED_CONST uint8_t side_value = ctree768[tree_record_pos*2 + checking_side];
        if(checking_side) { /* go right */
            tmp_code >>= 1; tmp_code |= 0x8000; /* ensure leading 1 */
        } else { /* go left */
            tmp_code >>= 1; tmp_code &= 0x7FFF; /* ensure leading 0 */
        }
        ++code_len;
        /**/
        if(ctree768[tree_record_pos + 512] & (checking_side ? 2 : 1)) { /* checked side ('left' or 'right') is leaf/value */
            /**/
            ECL_ASSERT(out_tspec512[side_value] == 0);
            out_tspec512[side_value] = ((tmp_code >> (16 - code_len)) & 0x0FFF) | (((uint16_t)code_len) << 12);
            /**/
            if(checking_side) { /* checked right - return back thru stack */
                uint8_t quit = 1;
                while(stack_depth) {
                    --stack_depth;
                    tree_record_pos = stack_ptrs[stack_depth];
                    tmp_code <<= 1; /* erase leading bit */
                    --code_len;
                    if(stack_flags[stack_depth] == 0) { /* 'left' was entered there - return to that record */
                        /* checking_side = 1; */ /* already 1 in this codepath */
                        tmp_code <<= 1; /* erase leading bit */
                        --code_len;
                        quit = 0;
                        break;
                    } /* else - 'right' was entered there - keep returning deeper */
                }
                if(quit) {
                    break;
                }
            } else { /* else - go to checking 'right' */
                checking_side = 1;
                tmp_code <<= 1; /* erase leading bit */
                --code_len;
            }
        } else {
            if(stack_depth >= ECL_HUFF8_TREE_DEPTH_MAX_TSPEC512) {
                ECL_ASSERT(0);
                return 0;
            }
            stack_flags[stack_depth] = checking_side;
            stack_ptrs[stack_depth] = (uint8_t)tree_record_pos;
            ++stack_depth;
            tree_record_pos = side_value;
            checking_side = 0;
        }
    } while(1);
    return 1;
}



uint16_t ECL_Huff8_GetMaxDepthCTree768(const uint8_t* ctree768, uint8_t* buf512) {
    uint8_t* ECL_SCOPED_CONST stack_flags = buf512; /* 0: entered left; 1: entered right; allocate arrays within free buffer */
    uint8_t* ECL_SCOPED_CONST stack_ptrs = buf512 + 256; /* -:- same size */
    uint16_t tree_record_pos = 0; /* root */
    uint16_t stack_depth = 0;
    uint16_t result = 0;
    uint8_t checking_side = 0; /* 0/1 (left/right) */

#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if((ctree768 == NULL) || (buf512 == NULL)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif

    do {
        if(ctree768[tree_record_pos + 512] & (checking_side ? 2 : 1)) { /* checked side ('left' or 'right') is leaf/value */
            if(checking_side) { /* checked right - return back thru stack */
                uint8_t quit = 1;
                while(stack_depth) {
                    --stack_depth;
                    tree_record_pos = stack_ptrs[stack_depth];
                    if(stack_flags[stack_depth] == 0) { /* 'left' was entered there - return to that record */
                        /* checking_side = 1; */ /* already 1 in this codepath */
                        quit = 0;
                        break;
                    } /* else - 'right' was entered there - keep returning deeper */
                }
                if(quit) {
                    break;
                }
            } else { /* else - go to checking 'right' */
                checking_side = 1;
            }
        } else {
            ECL_ASSERT(stack_depth < 256);
            stack_flags[stack_depth] = checking_side;
            stack_ptrs[stack_depth] = (uint8_t)tree_record_pos;
            ++stack_depth;
            tree_record_pos = ctree768[tree_record_pos*2 + checking_side];
            checking_side = 0;
            if(stack_depth > result) {
                result = stack_depth;
            }
        }
    } while(1);
    return result + 1;
}

uint16_t ECL_Huff8_GetMaxDepthTSpec1024(const uint32_t* tspec1024) {
    uint16_t result = 0;
#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if(tspec1024 == NULL) {
        ECL_ASSERT(0);
        return 0;
    }
#endif
    for(int i = 0; i < 256; ++i) {
        ECL_SCOPED_CONST uint16_t n_bits = (uint16_t)((tspec1024[i] >> 24) & 0x0FF);
        if(n_bits > result) {
            result = n_bits;
        }
    }
    return result;
}

uint16_t ECL_Huff8_GetMaxDepthTSpec768(const uint16_t* tspec768) {
    const uint8_t* ECL_SCOPED_CONST nbits_table = (const uint8_t*)(tspec768 + 256);
    uint16_t result = 0;
#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if(tspec768 == NULL) {
        ECL_ASSERT(0);
        return 0;
    }
#endif
    for(int i = 0; i < 256; ++i) {
        ECL_SCOPED_CONST uint16_t n_bits = nbits_table[i];
        if(n_bits > result) {
            result = n_bits;
        }
    }
    return result;
}

uint16_t ECL_Huff8_GetMaxDepthTSpec512(const uint16_t* tspec512) {
    uint16_t result = 0;
#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if(tspec512 == NULL) {
        ECL_ASSERT(0);
        return 0;
    }
#endif
    for(int i = 0; i < 256; ++i) {
        ECL_SCOPED_CONST uint16_t n_bits = (tspec512[i] >> 12) & 0x0F;
        if(n_bits > result) {
            result = n_bits;
        }
    }
    return result;
}


/* Evaluate/Analyze user methods - not modifying, estimate how much space is needed for compressed output ------------------------------------------ */

uint32_t ECL_Huff8_EvaluateTreeByN(uint16_t n_unique) {
    return ECL_HUFF8_COMPRESSED_TREE_SIZE_BITS(n_unique);
}

uint32_t ECL_Huff8_EvaluateTree(const uint8_t* ctree768) {
#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if(ctree768 == NULL) {
        ECL_ASSERT(0);
        return 0;
    }
#endif
    ECL_ASSERT(ctree768);
    return ECL_HUFF8_COMPRESSED_TREE_SIZE_BITS(((uint16_t)ctree768[767]) + 1);
}

uint32_t ECL_Huff8_Evaluate16_ForTSpec1024(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, const uint32_t* tspec1024/*[256]*/) {
    uint32_t result_bits = 0;
#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if((src == NULL) || (! bytes_cnt) || (! interval) || (tspec1024 == NULL)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif
    ECL_ASSERT(tspec1024 == ECL_GetAlignedConstPointer4((const uint8_t*)tspec1024));
    /**/
    for(uint32_t i = 0, ofs = 0; i < bytes_cnt; ++i, ofs += interval) {
        ECL_SCOPED_CONST uint32_t n_bits = (tspec1024[src[ofs]] >> 24) & 0x0FF;
        if(! n_bits) {
            return 0; /* not represented in the spec */
        }
        result_bits += n_bits;
    }
    return result_bits;
}

uint32_t ECL_Huff8_Evaluate16_ForTSpec768(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, const uint16_t* tspec768/*[768/2 == 384]*/) {
    const uint8_t* ECL_SCOPED_CONST nbits_table = (const uint8_t*)(tspec768 + 256);
    uint32_t result_bits = 0;
#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if((src == NULL) || (! bytes_cnt) || (! interval) || (tspec768 == NULL)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif
    ECL_ASSERT(tspec768 == ECL_GetAlignedConstPointer2((const uint8_t*)tspec768));
    /**/
    for(uint32_t i = 0, ofs = 0; i < bytes_cnt; ++i, ofs += interval) {
        ECL_SCOPED_CONST uint8_t n_bits = nbits_table[src[ofs]];
        if(! n_bits) {
            return 0; /* not represented in the spec */
        }
        result_bits += n_bits;
    }
    return result_bits;
}

uint32_t ECL_Huff8_Evaluate16_ForTSpec512(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, const uint16_t* tspec512/*[512/2 == 256]*/) {
    uint32_t result_bits = 0;
#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if((src == NULL) || (! bytes_cnt) || (! interval) || (tspec512 == NULL)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif
    ECL_ASSERT(tspec512 == ECL_GetAlignedConstPointer2((const uint8_t*)tspec512));
    /**/
    for(uint32_t i = 0, ofs = 0; i < bytes_cnt; ++i, ofs += interval) {
        ECL_SCOPED_CONST uint16_t n_bits = (tspec512[src[ofs]] >> 12) & 0x0F;
        if(! n_bits) {
            return 0; /* not represented in the spec */
        }
        result_bits += n_bits;
    }
    return result_bits;
}



uint32_t ECL_Huff8_Analyze16_ULM(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint32_t* buf1024, uint8_t* buf256, uint8_t* buf768) {
    uint16_t* ECL_SCOPED_CONST freqs_buf = (uint16_t*)buf1024;
    uint16_t n_unique;
    uint32_t result_bits;

#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if((src == NULL) || (! bytes_cnt) || (! interval) || (buf1024 == NULL) || (buf256 == NULL) || (buf768 == NULL)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif

    ECL_Huff8_FillFreqs16(src, bytes_cnt, interval, freqs_buf);
    n_unique = ECL_Huff8_Freqs16ToCTree768(freqs_buf, buf256, buf768, 0);

    if(n_unique < 2) {
        return 9; /* 9 bits */
    }
    ECL_Huff8_CTree768ToTSpec1024_ULM(buf768, buf1024, buf256);

    result_bits = (n_unique * 10) - 1;
    for(uint32_t i = 0, ofs = 0; i < bytes_cnt; ++i, ofs += interval) {
        ECL_SCOPED_CONST uint32_t n_bits = (buf1024[src[ofs]] >> 24) & 0x0FF;
        result_bits += n_bits;
    }
    return result_bits;
}

uint32_t ECL_Huff8_Analyze16_ULM_2k5(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint32_t* buf1024, uint8_t* buf256, uint8_t* buf768, uint16_t* buf512) {
    uint16_t* ECL_SCOPED_CONST freqs_buf = (uint16_t*)buf1024;
    uint16_t n_unique;
    uint32_t result_bits;

#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if((src == NULL) || (! bytes_cnt) || (! interval) || (buf1024 == NULL) || (buf256 == NULL) || (buf768 == NULL) || (buf512 == NULL)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif

    ECL_Huff8_FillFreqs16(src, bytes_cnt, interval, freqs_buf);
    memcpy(buf512, freqs_buf, 512);
    n_unique = ECL_Huff8_Freqs16ToCTree768(freqs_buf, buf256, buf768, 0);

    if(n_unique < 2) {
        return 9; /* 9 bits */
    }
    ECL_Huff8_CTree768ToTSpec1024_ULM(buf768, buf1024, buf256);

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

void ECL_Huff8_CompressCTree768(const uint8_t* ctree768, uint8_t* depth_buf_x2, uint16_t depth_buf_size, ECL_WSTREAM_Type* wstream) {
    uint8_t* ECL_SCOPED_CONST stack_flags = depth_buf_x2; /* 0: entered left; 1: entered right; allocate arrays within free buffer */
    uint8_t* ECL_SCOPED_CONST stack_ptrs = depth_buf_x2 + depth_buf_size; /* -:- same size */
    uint16_t tree_record_pos = 0; /* root */
    uint16_t stack_depth = 0;
    uint8_t checking_side = 0; /* 0/1 (left/right) */

#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if((ctree768 == NULL) || (depth_buf_x2 == NULL) || (! depth_buf_size) || (wstream == NULL)) {
        ECL_ASSERT(0);
        return;
    }
#endif

    if(! ctree768[767]) { /* n_unique == 1 */
        ECL_WSTREAM_Write1_8(wstream, 1, 1);
        ECL_WSTREAM_Write1_8(wstream, ctree768[766], 8);
        return;
    }
    ECL_WSTREAM_Write1_8(wstream, 0, 1);
    do {
        ECL_SCOPED_CONST uint8_t side_value = ctree768[tree_record_pos*2 + checking_side];
        /**/
        if(ctree768[tree_record_pos + 512] & (checking_side ? 2 : 1)) { /* checked side ('left' or 'right') is leaf/value */
            /**/
            ECL_WSTREAM_Write1_8(wstream, 1, 1);
            ECL_WSTREAM_Write1_8(wstream, side_value, 8);
            /**/
            if(checking_side) { /* checked right - return back thru stack */
                uint8_t quit = 1;
                while(stack_depth) {
                    --stack_depth;
                    tree_record_pos = stack_ptrs[stack_depth];
                    if(stack_flags[stack_depth] == 0) { /* 'left' was entered there - return to that record */
                        /* checking_side = 1; */ /* already 1 in this codepath */
                        quit = 0;
                        break;
                    } /* else - 'right' was entered there - keep returning deeper */
                }
                if(quit) {
                    break;
                }
            } else { /* else - go to checking 'right' */
                checking_side = 1;
            }
        } else {
            ECL_WSTREAM_Write1_8(wstream, 0, 1);
            /**/
            ECL_ASSERT(stack_depth < depth_buf_size);
            stack_flags[stack_depth] = checking_side;
            stack_ptrs[stack_depth] = (uint8_t)tree_record_pos;
            ++stack_depth;
            tree_record_pos = side_value;
            checking_side = 0;
        }
    } while(1);
}

/* compresses only data itself, returns amount of bits written (or 0 in case of error) */
uint32_t ECL_Huff8_CompressDataWithTSpec1024(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, const uint32_t* tspec1024/*[256]*/, ECL_WSTREAM_Type* wstream) {
    uint32_t result_bits = 0;

#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if((src == NULL) || (! bytes_cnt) || (! interval) || (tspec1024 == NULL) || (wstream == NULL)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif
    ECL_ASSERT(tspec1024 == ECL_GetAlignedConstPointer4((const uint8_t*)tspec1024));
    /**/
    for(uint32_t i = 0, ofs = 0; i < bytes_cnt; ++i, ofs += interval) {
        uint32_t info = tspec1024[src[ofs]];
        uint8_t n_bits = (uint8_t)(info >> 24);
        result_bits += n_bits;
        while(n_bits > 8) {
            ECL_WSTREAM_Write1_8(wstream, (uint8_t)info, 8);
            n_bits -= 8;
            info >>= 8;
        }
        ECL_WSTREAM_Write1_8(wstream, (uint8_t)info, n_bits);
    }
    return result_bits;
}

uint32_t ECL_Huff8_CompressDataWithTSpec768(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, const uint16_t* tspec768, ECL_WSTREAM_Type* wstream) {
    const uint8_t* ECL_SCOPED_CONST nbits_table = (const uint8_t*)(tspec768 + 256);
    uint32_t result_bits = 0;

#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if((src == NULL) || (! bytes_cnt) || (! interval) || (tspec768 == NULL) || (wstream == NULL)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif
    ECL_ASSERT(tspec768 == ECL_GetAlignedConstPointer2((const uint8_t*)tspec768));
    /**/
    for(uint32_t i = 0, ofs = 0; i < bytes_cnt; ++i, ofs += interval) {
        ECL_SCOPED_CONST uint8_t index = src[ofs];
        ECL_SCOPED_CONST uint16_t info = tspec768[index];
        ECL_SCOPED_CONST uint8_t n_bits = nbits_table[index];
        result_bits += n_bits;
        if(n_bits > 8) {
            ECL_WSTREAM_Write1_8(wstream, (uint8_t)info, 8);
            ECL_WSTREAM_Write1_8(wstream, (uint8_t)(info >> 8), (n_bits - 8));
        } else {
            ECL_WSTREAM_Write1_8(wstream, (uint8_t)info, n_bits);
        }
    }
    return result_bits;
}

uint32_t ECL_Huff8_CompressDataWithTSpec512(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, const uint16_t* tspec512, ECL_WSTREAM_Type* wstream) {
    uint32_t result_bits = 0;

#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if((src == NULL) || (! bytes_cnt) || (! interval) || (tspec512 == NULL) || (wstream == NULL)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif
    ECL_ASSERT(tspec512 == ECL_GetAlignedConstPointer2((const uint8_t*)tspec512));
    /**/
    for(uint32_t i = 0, ofs = 0; i < bytes_cnt; ++i, ofs += interval) {
        ECL_SCOPED_CONST uint16_t info = tspec512[src[ofs]];
        ECL_SCOPED_CONST uint8_t n_bits = (uint8_t)(info >> 12);
        result_bits += n_bits;
        if(n_bits > 8) {
            ECL_WSTREAM_Write1_8(wstream, (uint8_t)info, 8);
            ECL_WSTREAM_Write1_8(wstream, (uint8_t)(info >> 8), (n_bits - 8));
        } else {
            ECL_WSTREAM_Write1_8(wstream, (uint8_t)info, n_bits);
        }
    }
    return result_bits;
}



uint32_t ECL_Huff8_Compress16_ULM(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint32_t* buf1024/*[256]*/, uint8_t* buf256, uint8_t* buf768, ECL_WSTREAM_Type* wstream) {
    uint16_t* ECL_SCOPED_CONST freqs_buf = (uint16_t*)buf1024;
    uint16_t n_unique;
    uint32_t result_bits;

#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if((src == NULL) || (! bytes_cnt) || (! interval) || (buf1024 == NULL) || (buf256 == NULL) || (buf768 == NULL) || (wstream == NULL)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif

    ECL_ASSERT(buf1024 == ECL_GetAlignedPointer4((uint8_t*)buf1024));

    ECL_Huff8_FillFreqs16(src, bytes_cnt, interval, freqs_buf);
    n_unique = ECL_Huff8_Freqs16ToCTree768(freqs_buf, buf256, buf768, 0);

    result_bits = ECL_Huff8_EvaluateTreeByN(n_unique);
    ECL_Huff8_CompressCTree768(buf768, buf256, 128, wstream);

    if(n_unique < 2) {
        return result_bits;
    }
    ECL_Huff8_CTree768ToTSpec1024_ULM(buf768, buf1024, buf256);
    result_bits += ECL_Huff8_CompressDataWithTSpec1024(src, bytes_cnt, interval, buf1024, wstream);
    return result_bits;
}

uint32_t ECL_Huff8_TryCompress16_TSpec768(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint16_t* buf800/*[400]*/, uint8_t* buf768, ECL_WSTREAM_Type* wstream) {
    uint16_t* ECL_SCOPED_CONST freqs_buf = buf800;
    uint8_t* ECL_SCOPED_CONST buf256 = ((uint8_t*)buf800) + 512;
    uint16_t n_unique;
    uint32_t result_bits;

#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if((src == NULL) || (! bytes_cnt) || (! interval) || (buf800 == NULL) || (buf768 == NULL) || (wstream == NULL)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif
    ECL_ASSERT(buf800 == ECL_GetAlignedPointer2((uint8_t*)buf800));

    ECL_Huff8_FillFreqs16(src, bytes_cnt, interval, freqs_buf);
    n_unique = ECL_Huff8_Freqs16ToCTree768(freqs_buf, buf256, buf768, 0);

    result_bits = ECL_Huff8_EvaluateTreeByN(n_unique);
    ECL_Huff8_CompressCTree768(buf768, buf256, 128, wstream);

    if(n_unique < 2) {
        return result_bits;
    }
    if(! ECL_Huff8_CTree768ToTSpec768(buf768, buf800, ((uint8_t*)buf800) + 768)) {
        return 0; /* maximum tree depth exceeds TSpec768 capacity */
    }
    result_bits += ECL_Huff8_CompressDataWithTSpec768(src, bytes_cnt, interval, buf800, wstream);
    return result_bits;
}

uint32_t ECL_Huff8_TryCompress16_TSpec512(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint16_t* buf536/*[268]*/, uint8_t* buf256, uint8_t* buf768, ECL_WSTREAM_Type* wstream) {
    uint16_t* ECL_SCOPED_CONST freqs_buf = buf536;
    uint16_t n_unique;
    uint32_t result_bits;

#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if((src == NULL) || (! bytes_cnt) || (! interval) || (buf536 == NULL) || (buf256 == NULL) || (buf768 == NULL) || (wstream == NULL)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif
    ECL_ASSERT(buf536 == ECL_GetAlignedPointer2((uint8_t*)buf536));

    ECL_Huff8_FillFreqs16(src, bytes_cnt, interval, freqs_buf);
    n_unique = ECL_Huff8_Freqs16ToCTree768(freqs_buf, buf256, buf768, 0);

    result_bits = ECL_Huff8_EvaluateTreeByN(n_unique);
    ECL_Huff8_CompressCTree768(buf768, buf256, 128, wstream);

    if(n_unique < 2) {
        return result_bits;
    }
    if(! ECL_Huff8_CTree768ToTSpec512(buf768, buf536, ((uint8_t*)buf536) + 512)) {
        return 0; /* maximum tree depth exceeds TSpec512 capacity */
    }
    result_bits += ECL_Huff8_CompressDataWithTSpec512(src, bytes_cnt, interval, buf536, wstream);
    return result_bits;
}



#ifdef ECL_WSTREAM_JHx_Init

uint32_t ECL_Huff8_Compress16_ULM_Raw(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint32_t* buf1024/*[256]*/, uint8_t* buf256, uint8_t* buf768, uint8_t* dst, ECL_usize dst_size) {
    uint32_t result;
    ECL_WSTREAM_Type wstream;
    ECL_WSTREAM_JHx_Init(&wstream, dst, dst_size);
    ECL_ASSERT(bytes_cnt && interval);
    result = ECL_Huff8_Compress16_ULM(src, bytes_cnt, interval, buf1024, buf256, buf768, &wstream);
    if(! wstream.is_valid) { /* hardcoded JH-relation guaranteed by ECL_WSTREAM_JHx_Init presence */
        return 0;
    }
    return result;
}

uint32_t ECL_Huff8_TryCompress16_TSpec768_Raw(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint16_t* buf800/*[400]*/, uint8_t* buf768, uint8_t* dst, ECL_usize dst_size) {
    uint32_t result;
    ECL_WSTREAM_Type wstream;
    ECL_WSTREAM_JHx_Init(&wstream, dst, dst_size);
    ECL_ASSERT(bytes_cnt && interval);
    result = ECL_Huff8_TryCompress16_TSpec768(src, bytes_cnt, interval, buf800, buf768, &wstream);
    if(! wstream.is_valid) { /* hardcoded JH-relation guaranteed by ECL_WSTREAM_JHx_Init presence */
        return 0;
    }
    return result;
}

uint32_t ECL_Huff8_TryCompress16_TSpec512_Raw(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint16_t* buf536/*[268]*/, uint8_t* buf256, uint8_t* buf768, uint8_t* dst, ECL_usize dst_size) {
    uint32_t result;
    ECL_WSTREAM_Type wstream;
    ECL_WSTREAM_JHx_Init(&wstream, dst, dst_size);
    ECL_ASSERT(bytes_cnt && interval);
    result = ECL_Huff8_TryCompress16_TSpec512(src, bytes_cnt, interval, buf536, buf256, buf768, &wstream);
    if(! wstream.is_valid) { /* hardcoded JH-relation guaranteed by ECL_WSTREAM_JHx_Init presence */
        return 0;
    }
    return result;
}

#endif



/* Decompress* user methods ------------------------------------------------------------------------------------------------------------------------ */

int16_t ECL_Huff8_DecompressDTree1024(ECL_RSTREAM_Type* rstream, uint16_t* dtree_buf, uint16_t max_nodes/* <= 512*/) {
    /* dtree node is 16 bit. either 0x80_value_code or 0x0_right_node_index (0x0_right_node_index takes 9 bits) */
    uint16_t unp_stack[ECL_HUFF8_DECOMPRESS_MAX_DEPTH];
    uint16_t next_node = 1;
    uint16_t stack_depth = 0;
    uint16_t curr_node = 0;

#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if((rstream == NULL) || (dtree_buf == NULL) || (! max_nodes)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif
    if(max_nodes < 2) {
        return -1;
    }
    if(max_nodes > 512) {
        max_nodes = 512;
    }
    --max_nodes; /* 0 will be written in the end as a marker */
    /**/
    while(1) {
        if(ECL_RSTREAM_Read1_8(rstream, 1)) { /* leaf */
            uint16_t code = (uint16_t)ECL_RSTREAM_Read1_8(rstream, 8);
            dtree_buf[curr_node] = 0x8000 | code;
            while(stack_depth) { /* return to parent and check it's state */
                --stack_depth;
                curr_node = unp_stack[stack_depth];
                if(! dtree_buf[curr_node]) { /* left side done, go right */
                    ++stack_depth;
                    if(next_node >= max_nodes) {
                        return -4; /* ERROR: invalid data (tree branches more than for 256 codes) */
                    }
                    dtree_buf[curr_node] = next_node; /* ref right node */
                    curr_node = next_node; /* allocate right node */
                    ++next_node;
                    break;
                } /* else - right side done - move up */
            }
            if(! curr_node) {
                break; /* done, tree is unpacked */
            }
        } else { /* branching */
            if(stack_depth >= ECL_HUFF8_DECOMPRESS_MAX_DEPTH) {
                return -2; /* ERROR, either invalid data or not supported tree depth */
            }
            if(next_node >= max_nodes) {
                return -3; /* ERROR: invalid data (tree branches more than for 256 codes) */
            }
            unp_stack[stack_depth] = curr_node;
            ++stack_depth;
            dtree_buf[curr_node] = 0; /* '0' is a state before entering 'left' part (possible only during tree unpacking) */
            curr_node = next_node;
            ++next_node;
            /* go left */
        }
    } /* tree unpacked */
    dtree_buf[next_node] = 0; /* such thing will allow easy finding the end of dtree_buf for an algorithm accepting tree as parameter */
    return (int16_t)next_node;
}

ECL_usize ECL_Huff8_DecompressWithDTree1024(const uint16_t* dtree1024, ECL_RSTREAM_Type* rstream, uint8_t* dst, ECL_usize bytes_cnt, ECL_usize interval) {
#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if((dtree1024 == NULL) || (rstream == NULL) || (dst == NULL) || (! bytes_cnt) || (! interval)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif
    if(! bytes_cnt) {
        ECL_ASSERT(false && "Compressed Huffman block can't be zero-length!");
        return 0;
    }
    if(! dtree1024[0]) {
        return 0; /* error: no tree */
    }
    if(! dtree1024[1]) {
        uint8_t code = (uint8_t)dtree1024[0];
        /* only single code in tree - fill data with it */
        for(ECL_usize i = 0, ofs = 0; i < bytes_cnt; ++i, ofs += interval) {
            dst[ofs] = code;
        }
        return bytes_cnt; /* ok */
    }
    /* else - process as regular tree */
#ifdef ECL_HUFF8_DECOMPRESS_CACHE_BITS_READING
    {
        uint8_t nbits, val;
        ECL_usize ofs;
        ECL_SCOPED_CONST ECL_usize last_ofs = (interval * bytes_cnt);
        ofs = 0;
        nbits = 0;
        /**/
        if(bytes_cnt > 8) { /* leave last 8 values for separate loop - they will take AT LEAST 8 bits which we pre-read */
            ECL_SCOPED_CONST ECL_usize bound_ofs = (interval * (bytes_cnt - 8));
#ifdef ECL_RSTREAM_PeekWithinByte
            ECL_RSTREAM_PeekWithinByte(rstream, &nbits); /* get amount of bits in current byte - prefer aligning bound before entering loop */
#else /* --------------------------- */
            nbits = 8;
#endif /* ECL_RSTREAM_PeekWithinByte */
            val = ECL_RSTREAM_Read1_8(rstream, nbits);
            /**/
            for(; ofs < bound_ofs; ofs += interval) {
                uint16_t curr_node = 0; /* we're sure it's not a leaf (checked that case above) */
                do {
                    if(! nbits) {
                        val = ECL_RSTREAM_Read1_8(rstream, 8);
                        nbits = 8;
                    }
                    if(val & 1) { /* go right */
                        curr_node = dtree1024[curr_node];
                    } else { /* go left (always next after parent) */
                        ++curr_node;
                    }
                    val >>= 1;
                    --nbits;
                } while(! (dtree1024[curr_node] & 0x8000));
                dst[ofs] = (uint8_t)dtree1024[curr_node];
            }
        }
        /* complete last part with per-bit reading */
        for(; ofs < last_ofs; ofs += interval) {
            uint16_t curr_node = 0;
            do {
                if(! nbits) {
                    val = ECL_RSTREAM_Read1_8(rstream, 1);
                    nbits = 1;
                }
                if(val & 1) { /* go right */
                    curr_node = dtree1024[curr_node];
                } else { /* go left (always next after parent) */
                    ++curr_node;
                }
                val >>= 1;
                --nbits;
            } while(! (dtree1024[curr_node] & 0x8000));
            dst[ofs] = (uint8_t)dtree1024[curr_node];
        }
    }
#else /* non-cached */
    for(ECL_usize last_ofs = (interval * bytes_cnt), ofs = 0; ofs < last_ofs; ofs += interval) {
        uint16_t curr_node = 0; /* we're sure it's not a leaf (checked that case above) */
        do {
            if(ECL_RSTREAM_Read1_8(rstream, 1)) { /* go right */
                curr_node = dtree1024[curr_node];
            } else { /* go left (always next after parent) */
                ++curr_node;
            }
        } while(! (dtree1024[curr_node] & 0x8000));
        dst[ofs] = (uint8_t)dtree1024[curr_node];
    }
#endif
    return bytes_cnt; /* ok if stream isn't invalidated */
}

ECL_usize ECL_Huff8_Decompress(ECL_RSTREAM_Type* rstream, uint16_t* dtree_buf/*[512]*/, uint8_t* dst, ECL_usize bytes_cnt, ECL_usize interval) {
    ECL_ASSERT(bytes_cnt && interval);
    if(ECL_Huff8_DecompressDTree1024(rstream, dtree_buf, 512) <= 0) {
        return 0;
    }
    return ECL_Huff8_DecompressWithDTree1024(dtree_buf, rstream, dst, bytes_cnt, interval);
}

int16_t ECL_Huff8_DTree1024ToDTable768(const uint16_t* dtree1024, uint16_t* dtable768/*[768/2 == 384]*/) {
    uint16_t unp_stack[8];
    uint16_t* ECL_SCOPED_CONST out_codes_table = dtable768;
    uint8_t* ECL_SCOPED_CONST out_nbits_table = (uint8_t*)(dtable768 + 256);
    uint8_t tmp_code = 0; /* lower bit is entering bit in the code */
    uint16_t curr_node = 1;
    uint16_t stack_depth = 1; /* root entered left */
    unp_stack[0] = 0;

    if(! dtree1024[0]) {
        return 0; /* error: no tree */
    }
    if(! dtree1024[1]) {
        return -1; /* single-element - not reasonable and not supported by algorithm */
    }

    while(stack_depth) {
        if(dtree1024[curr_node] & 0x8000) { /* leaf */
            /* form table */
            ECL_SCOPED_CONST uint8_t increment = (uint8_t)(1 << stack_depth); /* can be 0 */
            ECL_SCOPED_CONST uint8_t orig_code = tmp_code;
            ECL_SCOPED_CONST uint8_t value = (uint8_t)(dtree1024[curr_node]);
            do {
                out_codes_table[tmp_code] = value;
                out_nbits_table[tmp_code] = (uint8_t)stack_depth;
                tmp_code += increment;
            } while(tmp_code != orig_code);
        } else if(stack_depth == 8) { /* stack_depth(==code length in bits) is 8, not a leaf */
            /* form table */
            out_codes_table[tmp_code] = curr_node; /* ref in dtree1024 to continue traversing from */
            out_nbits_table[tmp_code] = 9; /* or more */
        } else { /* go deeper */
            tmp_code &= ~(uint8_t)(1 << stack_depth); /* enter 'left' -> add '0' as higher bit */
            unp_stack[stack_depth] = curr_node;
            ++stack_depth;
            ++curr_node; /* enter 'left' */
            continue;
        }
        /* move level up after forming table */
        while(stack_depth) {
            --stack_depth;
            curr_node = unp_stack[stack_depth];
            if(! (tmp_code & (uint8_t)(1 << stack_depth))) { /* entered left at that level */
                tmp_code |= (uint8_t)(1 << stack_depth); /* enter 'right' -> add '1' as higher bit */
                /* unp_stack[stack_depth] = curr_node; */ /* already there */
                curr_node = dtree1024[curr_node];
                ++stack_depth;
                break;
            }
        }
    }
    return 1;
}

ECL_usize ECL_Huff8_DecompressWithDTable768(const uint16_t* dtree1024, const uint16_t* dtable768/*[768/2 == 384]*/, ECL_RSTREAM_Type* rstream, uint8_t* dst, ECL_usize bytes_cnt, ECL_usize interval) {
    uint8_t nbits;
    uint16_t val;
    ECL_usize ofs;
    ECL_SCOPED_CONST ECL_usize last_ofs = (interval * bytes_cnt);
#ifndef ECL_HUFF8_DISABLE_NULL_CHECKS
    if((dtree1024 == NULL) || (dtable768 == NULL) || (rstream == NULL) || (dst == NULL) || (! bytes_cnt) || (! interval)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif
    ofs = 0;
    nbits = 0;
    /**/
    if(bytes_cnt > 16) {
        const uint16_t* ECL_SCOPED_CONST codes_table = dtable768;
        const uint8_t* ECL_SCOPED_CONST nbits_table = (uint8_t*)(dtable768 + 256);
        ECL_SCOPED_CONST ECL_usize bound_ofs = (interval * (bytes_cnt - 16)); /* leave last 16 values for separate loop - they will take AT LEAST 16 bits which we pre-read */

#ifdef ECL_RSTREAM_PeekWithinByte
        ECL_RSTREAM_PeekWithinByte(rstream, &nbits); /* get amount of bits in current byte - prefer aligning bound before entering loop */
#else /* --------------------------- */
        nbits = 8;
#endif /* ECL_RSTREAM_PeekWithinByte */
        val = ECL_RSTREAM_Read1_8(rstream, nbits);
        /**/
        for(; ofs < bound_ofs; ofs += interval) {
            uint8_t code_size;
            if(nbits < 8) { /* make sure we have >= 8 bits to use as index */
                val |= ((uint16_t)ECL_RSTREAM_Read1_8(rstream, 8)) << nbits;
                nbits += 8;
            }
            code_size = nbits_table[(uint8_t)val]; /* can't be 0 */
            if(code_size <= 8) {
                dst[ofs] = (uint8_t)codes_table[(uint8_t)val];
                val >>= code_size;
                nbits -= code_size;
            } else {
                uint16_t curr_node = codes_table[(uint8_t)val]; /* start search from that node */
                val >>= 8;
                nbits -= 8;
                do {
                    if(nbits < 8) {
                        val |= ((uint16_t)ECL_RSTREAM_Read1_8(rstream, 8)) << nbits;
                        nbits += 8;
                    }
                    if(val & 1) { /* go right */
                        curr_node = dtree1024[curr_node];
                    } else { /* go left (always next after parent) */
                        ++curr_node;
                    }
                    val >>= 1;
                    --nbits;
                } while(! (dtree1024[curr_node] & 0x8000));
                dst[ofs] = (uint8_t)dtree1024[curr_node];
            }
        }
    }
    /* complete last part with per-bit reading */
    for(; ofs < last_ofs; ofs += interval) {
        uint16_t curr_node = 0;
        do {
            if(! nbits) {
                val = ECL_RSTREAM_Read1_8(rstream, 1);
                nbits = 1;
            }
            if(val & 1) { /* go right */
                curr_node = dtree1024[curr_node];
            } else { /* go left (always next after parent) */
                ++curr_node;
            }
            val >>= 1;
            --nbits;
        } while(! (dtree1024[curr_node] & 0x8000));
        dst[ofs] = (uint8_t)dtree1024[curr_node];
    }
    return bytes_cnt;
}

#ifdef ECL_RSTREAM_JHx_Init

ECL_usize ECL_Huff8_Decompress_Raw(const uint8_t* src, ECL_usize src_size, uint16_t* dtree_buf/*[512]*/, uint8_t* dst, ECL_usize bytes_cnt, ECL_usize interval) {
    ECL_RSTREAM_Type rstream;
    ECL_RSTREAM_JHx_Init(&rstream, src, src_size);
    ECL_ASSERT(bytes_cnt && interval);

    if(ECL_Huff8_DecompressDTree1024(&rstream, dtree_buf, 512) <= 0) {
        return 0;
    }
    if(! ECL_Huff8_DecompressWithDTree1024(dtree_buf, &rstream, dst, bytes_cnt, interval)) {
        return 0;
    }
    /* hardcoded JH-relations guaranteed by ECL_RSTREAM_JHx_Init presence */
    if(! rstream.is_valid) {
        return 0;
    }
    return (uintptr_t)(rstream.next - src);
}

ECL_usize ECL_Huff8_DecompressWithDTable768_Raw(const uint8_t* src, ECL_usize src_size, uint16_t* dtree_buf/*[512]*/, uint16_t* dtable_buf/*[768/2 == 384]*/, uint8_t* dst, ECL_usize bytes_cnt, ECL_usize interval) {
    int16_t dtree_result;
    ECL_RSTREAM_Type rstream;
    ECL_RSTREAM_JHx_Init(&rstream, src, src_size);
    ECL_ASSERT(bytes_cnt && interval);

    dtree_result = ECL_Huff8_DecompressDTree1024(&rstream, dtree_buf, 512);
    if(dtree_result <= 0) {
        return 0;
    }
    if((dtree_result < 30) || (bytes_cnt < 200)) { /* not worth to use dtable - use default decompress */
        if(! ECL_Huff8_DecompressWithDTree1024(dtree_buf, &rstream, dst, bytes_cnt, interval)) {
            return 0;
        }
    } else { /* use dtable */
        if(ECL_Huff8_DTree1024ToDTable768(dtree_buf, dtable_buf) <= 0) {
            return 0;
        }
        if(! ECL_Huff8_DecompressWithDTable768(dtree_buf, dtable_buf, &rstream, dst, bytes_cnt, interval)) {
            return 0;
        }
    }
    /* hardcoded JH-relations guaranteed by ECL_RSTREAM_JHx_Init presence */
    if(! rstream.is_valid) {
        return 0;
    }
    return (uintptr_t)(rstream.next - src);
}

#endif
