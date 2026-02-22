/*
 * Copyright 2026 - 2026 Evgeniy Evstratov
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

#include "ECL_SLA.h"

/*************************************************************************************
   Sequence Levels Analyzer (SLA) algorithm designed and developed by Evgeniy Evstratov in 2012.
   Methods encode differencial signal best
   Methods don't like noise
   Analyzer should be called before coding (and return header in parameters, for coder)

    uses by 3 bits per coding high and low level in header.
    highlevel and lowlevel are decremented values of bit-counts per code without level determination bit (LDB),
    meaning (example): highlevel = 3, then high leveled code is [level determination bit][4 lower bits of data].
    1. highlevel > lowlevel -> default situation, +1 bit before code for determine high/or low level is now; (not using 0-bits coding)
    2. highlevel = lowlevel -> special coding, determination bit is not needed; (not using 0-bits coding)
    Extensions:
    3. highlevel = 0, lowlevel > 1 (2/3/4/5/6/7) -> high level is really 0 bits, lowlevel is increased:
    [level determination bit][(lowlevel-1) bits of data]) ; (using 0-bits coding).
    4. highlevel = 0, lowlevel = 1 -> means stream of zeroes (no data to write, except header).
    5. highlevel = 5, lowlevel > 5 (6,7) -> high level is 0 bits, lowlevel is DEcreased:
    [level determination bit][(lowlevel+1) bits of data]) ; (using 0-bits coding).

SLA header codes table (in cell - variant, or '*' if not used):
        highlevel ->
        | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
      |=|================================
    l |0| 2 | 1 | 1 | 1 | 1 | 1 | 1 | 1 |
    o -----------------------------------
    w |1| 4 | 2 | 1 | 1 | 1 | 1 | 1 | 1 |
    l -----------------------------------
    e |2| 3 | * | 2 | 1 | 1 | 1 | 1 | 1 |
    v -----------------------------------
    e |3| 3 | * | * | 2 | 1 | 1 | 1 | 1 |
    l -----------------------------------
      |4| 3 | * | * | * | 2 | 1 | 1 | 1 |
    | -----------------------------------
    \/|5| 3 | * | * | * | * | 2 | 1 | 1 |
      -----------------------------------
      |6| 3 | * | * | * | * | 5 | 2 | 1 |
      -----------------------------------
      |7| 3 | * | * | * | * | 5 | * | 2 |
      ===================================
    19 free cells.
*************************************************************************************/

static uint8_t c_ecl_sla_addbits[8] = {0xFE,0xFC,0xF8,0xF0,0xE0,0xC0,0x80,0x00}; /* map of sign-extension by value's bit count. used for decoding */

inline char ECL_SLA_AUX_MakeHDR(uint8_t high, uint8_t low) {
    return (char)( ((high)&0x07) | (((low)&0x07)<<3) );
}

inline uint8_t ECL_SLA_AUX_GetHL(char head) {
    return head & 0x07;
}

inline uint8_t ECL_SLA_AUX_GetLL(char head) {
    return (head>>3) & 0x07;
}

const uint8_t* ECL_SLA_GetTable() {
    static const uint8_t c_ecl_sla_table[] = {
        0,2,3,3,4,4,4,4,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,5,5,5,5,5,5,5,5,4,4,4,4,3,3,2,1
    };
    return c_ecl_sla_table;
}

ECL_usize ECL_SLA_Analyze(const uint8_t* src, ECL_usize size, char* dst_header) {
    /* returns size in bits : 3 + 3 + data_size */
    ECL_usize bits, bbest, bact; /* total bits ; bits best ; bits actual */
    uint8_t highlevel, lowlevel;
    uint32_t cntrs[9] = {0}; /* 0..8-bits counters */

#ifndef ECL_SLA_DISABLE_NULL_CHECKS
    if((src == NULL) || (! size) || (dst_header == NULL)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif
    bits = 3 + 3;
    if(! size) {
        return bits;
    }
    const uint8_t* sla_table = ECL_SLA_GetTable();

    for(ECL_usize index = 0; index < size; ++index) {
        ++cntrs[sla_table[src[index]]];
    } /* statistic done */
    char i;
    for(i = 8; i >= 0; --i) {
        if(cntrs[i]) {
            break;
        }
    }
    highlevel = i; /* high level is known */
    /* transforming cntrs */
    for(i = 1; i < highlevel; ++i) {
        cntrs[i] += cntrs[i-1];
    }
    /* done. highlevel contains REAL BIT COUNT per data (excluding possible LDB) */
    if(highlevel == 0) { /* all zeroes */
        *dst_header = ECL_SLA_AUX_MakeHDR(0, 1); /* variant 4, only header */
        return bits;
    } /* further this variant is excluded */
    lowlevel = highlevel;
    bbest = size * highlevel; /* if all - same-leveled (high = low, variant 2) */
    for(i = highlevel - 1; i >= 0; --i) {
        bact = cntrs[i] * (1+i); /* count * (LDB + value) */
        bact += (size - cntrs[i]) * (1 + highlevel); /* high-leveled values */
        if(bact < bbest) {
            bbest = bact;
            lowlevel = i;
        }
    }
    bits += bbest;
    if(lowlevel == 0) {
        if(highlevel < 7) { /* variant 3 */
            *dst_header = ECL_SLA_AUX_MakeHDR(0, highlevel+1);
        } else { /* variant 5 */
            *dst_header = ECL_SLA_AUX_MakeHDR(5, highlevel-1);
        }
    } else {
        *dst_header = ECL_SLA_AUX_MakeHDR(highlevel-1, lowlevel-1); /* variant 1 */
    }
    return bits;
} /* end analyze ---------------------------------------------------------------------------------- */

uint8_t ECL_SLA_Compress(const uint8_t* src, ECL_usize size, char pre_calc_header, ECL_WSTREAM_Type* wstream) {
    uint8_t lowl, highl, LLm1, HLm1;

#ifndef ECL_SLA_DISABLE_NULL_CHECKS
    if((src == NULL) || (! size) || (wstream == NULL)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif
    HLm1 = ECL_SLA_AUX_GetHL(pre_calc_header);
    LLm1 = ECL_SLA_AUX_GetLL(pre_calc_header);
    ECL_WSTREAM_Write1_8(wstream, pre_calc_header, 6);

    if(LLm1 > HLm1) { /* extension */
        if(LLm1 == 1) {
            return 1; /* variant 4 */
        }
        if(HLm1 == 0) { /* variant 3 */
            lowl = LLm1; /* including LDB */
        } else {
            if(HLm1 == 5) { /* variant 5 */
                lowl = LLm1+2; /* including LDB */
            }
        }
        if(lowl <= 8) {
            for(ECL_usize i = 0; i < size; ++i) {
                if(src[i] != 0) { /* as low level */
                    ECL_WSTREAM_Write1_8(wstream, (((src[i]) << 1) | 0x01), lowl);
                } else {
                    ECL_WSTREAM_Write1_8(wstream, 0, 1); /* LDB = 0 */
                }
            }
        } else {
            --lowl; /* exclude LDB - has to write LDB separately due to ECL_WSTREAM_Write1_8 restriction */
            for(ECL_usize i = 0; i < size; ++i) {
                if(src[i] != 0) { /* as low level */
                    ECL_WSTREAM_Write1_8(wstream, 1, 1); /* LDB = 1 */
                    ECL_WSTREAM_Write1_8(wstream, src[i], lowl);
                } else {
                    ECL_WSTREAM_Write1_8(wstream, 0, 1); /* LDB = 0 */
                }
            }
        }
    } else if(HLm1 != LLm1) { /* variant 1 */
        const uint8_t* sla_table = ECL_SLA_GetTable();
        lowl = LLm1 + 2; /* bits per low-level value + LDB */
        highl = HLm1 + 2; /* bits per high-level value + LDB */

        if((highl <= 8) && (lowl <= 8)) { /* can write with (LDB) safely using ECL_WSTREAM_Write1_8 */
            const uint8_t levels[2] = {highl, lowl};
            for(ECL_usize i = 0; i < size; ++i) {
                const uint8_t is_low = (uint8_t)(sla_table[src[i]] - lowl) >> 7;
                ECL_WSTREAM_Write1_8(wstream, ((src[i] << 1) | is_low), levels[is_low]);
            }
        } else { /* need to write LDB separately */
            const uint8_t levels[2] = {(uint8_t)(highl - 1), (uint8_t)(lowl - 1)};
            for(ECL_usize i = 0; i < size; ++i) {
                const uint8_t is_low = (uint8_t)(sla_table[src[i]] - lowl) >> 7;
                ECL_WSTREAM_Write1_8(wstream, is_low, 1);
                ECL_WSTREAM_Write1_8(wstream, src[i], levels[is_low]);
            }
        }
    } else { /* same level, variant 2 */
        highl = HLm1 + 1; /* bits per any value */
        for(ECL_usize i = 0; i < size; ++i) {
            ECL_WSTREAM_Write1_8(wstream, src[i], highl);
        }
    }
    return 1;
} /* end compress ---------------------------------------------------------------------------------- */

uint8_t ECL_SLA_Decompress(ECL_RSTREAM_Type* rstream, uint8_t* dst, ECL_usize size) {
    uint8_t HLm1, LLm1;
    uint8_t lowl, highl, head; /* low level bits, high level bits, header */

#ifndef ECL_SLA_DISABLE_NULL_CHECKS
    if((rstream == NULL) || (dst == NULL) || (! size)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif
    head = ECL_RSTREAM_Read1_8(rstream, 6);
    highl = HLm1 = ECL_SLA_AUX_GetHL(head);
    lowl = LLm1 = ECL_SLA_AUX_GetLL(head);

    if(LLm1 > HLm1) { /* extension */
        if(LLm1 == 1) {
            memset(dst, 0, size);
            return 1;
        }
        if(HLm1 == 0) { /* variant 3 */
            lowl = LLm1 - 1;
        } else {
            if(HLm1 == 5) { /* variant 5 */
                lowl = LLm1 + 1;
            }
        }
        LLm1 = lowl - 1;
        for(ECL_usize i = 0; i < size; ++i) {
            if(ECL_RSTREAM_Read1_8(rstream, 1)) { /* low-level */
                const uint8_t value = ECL_RSTREAM_Read1_8(rstream, lowl);
                dst[i] = value | (-(value >> LLm1) & c_ecl_sla_addbits[LLm1]); /* hardcore expanding with sign-bit */
            } else {
                dst[i] = 0; /* high-level, simply zero */
            }
        }
    } else if(HLm1 != LLm1) { /* standard coding, variant 1 */
        ++highl;
        ++lowl;
        const uint8_t to_read[2] = {highl, lowl};
        const uint8_t to_check[2] = {HLm1, LLm1};
        for(ECL_usize i = 0; i < size; ++i) {
            const uint8_t LDB = ECL_RSTREAM_Read1_8(rstream, 1);
            const uint8_t value = ECL_RSTREAM_Read1_8(rstream, to_read[LDB]);
            const auto nbits = to_check[LDB];
            dst[i] = value | (-(value >> nbits) & c_ecl_sla_addbits[nbits]);
        }
    } else { /* special coding, variant 2 */
        ++highl;
        for(ECL_usize i = 0; i < size; ++i) {
            const uint8_t value = ECL_RSTREAM_Read1_8(rstream, highl);
            dst[i] = value | (-(value >> HLm1) & c_ecl_sla_addbits[HLm1]);
        }
    }
    return 1;
} /* end decompress ---------------------------------------------------------------------------------- */



#ifdef ECL_WSTREAM_JHx_Init
uint8_t ECL_SLA_Compress_Raw(const uint8_t* src, ECL_usize src_size, char pre_calc_header, uint8_t* dst, ECL_usize dst_size) {
    ECL_WSTREAM_Type wstream;
    ECL_WSTREAM_JHx_Init(&wstream, dst, dst_size);
    if((! ECL_SLA_Compress(src, src_size, pre_calc_header, &wstream)) || (! wstream.is_valid)) {
        return 0;
    }
    return 1;
}
#endif /* ECL_WSTREAM_JHx_Init */


#ifdef ECL_RSTREAM_JHx_Init
uint8_t ECL_SLA_Decompress_Raw(const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize size) {
    ECL_RSTREAM_Type rstream;
    ECL_RSTREAM_JHx_Init(&rstream, src, src_size);
    if((! ECL_SLA_Decompress(&rstream, dst, size)) || (! rstream.is_valid)) {
        return 0;
    }
    return 1;
}
#endif /* ECL_RSTREAM_JHx_Init */


/* ----------------------------------------------------------------------------------------------------*/

uint8_t ECL_SLA_PackU16(const uint16_t* src, ECL_usize block_offset, uint16_t n_values, uint8_t* buffers) {
#ifndef ECL_SLA_DISABLE_NULL_CHECKS
    if((src == NULL) || (! n_values) || (buffers == NULL)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif

    src += block_offset;
    {
        uint16_t val_0 = block_offset
            ? (*src - src[-1])
            : *src
            ;
        *buffers = (uint8_t)(val_0);
        buffers[n_values] = (uint8_t)(val_0 >> 8);
    }
    {
        uint8_t* ECL_SCOPED_CONST buf_bound = buffers + n_values;
        for(++buffers; buffers < buf_bound; ++buffers, ++src) {
            ECL_SCOPED_CONST uint16_t s_diff = (uint16_t)(src[1] - *src);

            *buffers          = (uint8_t)(s_diff);
            buffers[n_values] = (uint8_t)(s_diff >> 8);
        }
    }
    return 1;
}

uint8_t ECL_SLA_UnpackU16(const uint8_t* buffers, uint16_t* dst, ECL_usize block_offset, uint16_t n_values) {
#ifndef ECL_SLA_DISABLE_NULL_CHECKS
    if((buffers == NULL) || (dst == NULL) || (! n_values)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif

    dst += block_offset;
    *dst = (uint16_t)(*buffers) | (((uint16_t)(buffers[n_values])) << 8);
    if(block_offset) {
        *dst += dst[-1];
    }
    {
        const uint8_t* ECL_SCOPED_CONST buf_bound = buffers + n_values;
        for(++buffers; buffers < buf_bound; ++buffers, ++dst) {
            dst[1] = ((uint16_t)(*buffers) | ((uint16_t)(buffers[n_values]) << 8)) + *dst;
        }
    }
    return 1;
}

/**/

uint8_t ECL_SLA_PackU32(const uint32_t* src, ECL_usize block_offset, uint16_t n_values, uint8_t* buffers) {
#ifndef ECL_SLA_DISABLE_NULL_CHECKS
    if((src == NULL) || (! n_values) || (buffers == NULL)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif

    src += block_offset;
    {
        uint32_t val_0 = block_offset
            ? (*src - src[-1])
            : *src
            ;
        *buffers            = (uint8_t)(val_0);
        buffers[n_values]   = (uint8_t)(val_0 >> 8);
        buffers[n_values*2] = (uint8_t)(val_0 >> 16);
        buffers[n_values*3] = (uint8_t)(val_0 >> 24);
    }
    {
        uint8_t* ECL_SCOPED_CONST buf_bound = buffers + n_values;
        for(++buffers; buffers < buf_bound; ++buffers, ++src) {
            ECL_SCOPED_CONST uint32_t s_diff = (uint32_t)(src[1] - *src);

            *buffers            = (uint8_t)(s_diff);
            buffers[n_values  ] = (uint8_t)(s_diff >> 8);
            buffers[n_values*2] = (uint8_t)(s_diff >> 16);
            buffers[n_values*3] = (uint8_t)(s_diff >> 24);
        }
    }
    return 1;
}

uint8_t ECL_SLA_UnpackU32(const uint8_t* buffers, uint32_t* dst, ECL_usize block_offset, uint16_t n_values) {
#ifndef ECL_SLA_DISABLE_NULL_CHECKS
    if((buffers == NULL) || (dst == NULL) || (! n_values)) {
        ECL_ASSERT(0);
        return 0;
    }
#endif

    dst += block_offset;
    *dst = (uint32_t)(*buffers)
        | (((uint32_t)(buffers[n_values])) << 8)
        | (((uint32_t)(buffers[n_values*2])) << 16)
        | (((uint32_t)(buffers[n_values*3])) << 24)
        ;
    if(block_offset) {
        *dst += dst[-1];
    }
    {
        const uint8_t* ECL_SCOPED_CONST buf_bound = buffers + n_values;
        for(++buffers; buffers < buf_bound; ++buffers, ++dst) {
            dst[1] = ( (uint32_t)(*buffers)
                   | (((uint32_t)(buffers[n_values])) << 8)
                   | (((uint32_t)(buffers[n_values*2])) << 16)
                   | (((uint32_t)(buffers[n_values*3])) << 24)
                   ) + *dst;
        }
    }
    return 1;
}
