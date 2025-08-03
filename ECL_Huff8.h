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

#ifndef ECL_HUFF8_
#define ECL_HUFF8_

#include "ECL_config.h"

#include <stdbool.h>

/* input(R*) and output(W*) streams and functions are used thru defines so they can be overridden if different work with memory is needed */
#ifndef ECL_HUFF8_RSTREAM_Type
    #define ECL_HUFF8_RSTREAM_Type ECL_JH_RState
    #define ECL_HUFF8_RSTREAM_Read1_8 ECL_JH_Read /* ECL_HUFF8_RSTREAM_Read1_8(ECL_HUFF8_RSTREAM_Type, n_bits) where n_bits is 1..8 */
    #define ECL_HUFF8_RSTREAM_Init(rstream, src, size) ECL_JH_RInit(rstream, src, size, 0) /* standard init of ECL_JH_RState for *_Raw functions - those are disabled if macro isn't defined */
#endif

#ifndef ECL_HUFF8_WSTREAM_Type
    #define ECL_HUFF8_WSTREAM_Type ECL_JH_WState
    #define ECL_HUFF8_WSTREAM_Write1_8 ECL_JH_Write /* ECL_HUFF8_WSTREAM_Write1_8(ECL_HUFF8_WSTREAM_Type, value, n_bits) where n_bits is 1..8 */
    #define ECL_HUFF8_WSTREAM_Init(wstream, dst, size) ECL_JH_WInit(wstream, dst, size, 0) /* standard init of ECL_JH_WState for *_Raw functions - those are disabled if macro isn't defined */
#endif


#define ECL_HUFF8_TREE_DEPTH_MAX_ULM 24

/* TODO description */
#ifndef ECL_HUFF8_DECOMPRESS_MAX_DEPTH
    #define ECL_HUFF8_DECOMPRESS_MAX_DEPTH ECL_HUFF8_TREE_DEPTH_MAX_ULM
#endif


/*
    Calculates size of buffer to fit compressed version of any data of 'src_size' size.
*/
#define ECL_HUFF8_GET_BOUND(src_size) ((src_size) + 1 + 320) // TODO make sure





#ifdef __cplusplus
extern "C" {
#endif

/* Auxiliary methods (called internally, for advanced users) --------------------------------------------------------------------------------------- */

/*
    Generates tree and spec from freqs.
    'freqs' is input array uint16_t[512] with first 256 values [i] filled with counts off appearance of codes 'i' in input data. rest 256 values states doesn't matter.
    'out_tspec1024' is output spec data in tspec1024 format: 256 uint32_t values (aligned allocation), each value[i] representing info about how to encode [i]:
        - top 8 bits is uint8_t amount of bits in the bitcode;
        - lower 24 bits is bitcode;
    'freqs' and 'out_tspec1024' can point to same address, in this case original freqs are being overriden.
    'buf256' is a 256-byte buffer to be supplied by user for internal needs.
    'out_ctree768' is two arrays in series:
        [{uint8_t left, uint8_t right}, ...] : 512 bytes; left/right are equally encoded and represent either an index of child node or value(code) depending of flags;
        [{uint8_t flags}, ...] : 256 bytes; indices match indices of records of previous array; if bit#0==1 then 'left' represents code, bit#1 is same for 'right';
        represents an encoded tree data needed for further processing, meaningful only if return value is > 1.
        Root code/record is top node, which has logical address of ('return value' - 2);
    returns amount of unique values (where freqs[i] are non-zero).
*/
ECL_EXPORTED_API int16_t ECL_Huff8_Freqs16ToTSpec1024_ULM(uint16_t* freqs/*[512]*/, uint32_t* out_tspec1024/*[256]*/, uint8_t* buf256, uint8_t* out_ctree768, uint16_t n_unique_max);



/* Evaluate/Analyze user methods - not modifying, estimate how much space is needed for compressed output ------------------------------------------ */

// returns amount of bits needed to encode a tree for 'n_unique' unique elements (or 0 in case of error)
ECL_EXPORTED_API uint32_t ECL_Huff8_EvaluateTree(uint16_t n_unique);

/*
    TODO description
    returns amount of bits needed for compression (or 0 in case of error).
*/
ECL_EXPORTED_API uint32_t ECL_Huff8_Analyze16_ULM(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint32_t* buf1024/*[256]*/, uint8_t* buf256, uint8_t* buf768);

/*
    Same as 'ECL_Huff8_Analyze16_ULM' but with extra 512 bytes (uint16_t aligned) buffer, resulting in a bit better performance for bigger datasets.
    returns amount of bits needed for compression (or 0 in case of error).
*/
ECL_EXPORTED_API uint32_t ECL_Huff8_Analyze16_ULM_2k5(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint32_t* buf1024/*[256]*/, uint8_t* buf256, uint8_t* buf768, uint16_t* buf512);



/* Compress* user methods -------------------------------------------------------------------------------------------------------------------------- */

/*
    Compresses to wstream a tree in 'ctree768' format.
    // TODO more description
    - 'depth_buf_x2' is external buffer needed to traversing the tree - it could be smaller or bigger depending on user needs:
        - for ULM 'depth_buf_x2' size: 2 * 'depth_buf_size' = 2 * ECL_HUFF8_TREE_DEPTH_MAX_ULM; ULM covers any user data up to 64k bytes (actually more).
        - for tiny datasets depth=8 could be enough ('depth_buf_size' = 8, depth_buf_x2 is 16 bytes long);
        - for any technically possible tree (possible on absurdly enormous data size) 'depth_buf_size' = 256, depth_buf_x2 is 512 bytes;
        - ! 'depth_buf_x2' buffer overflow isn't protected (with if/return), only checked with ECL_ASSERT;
*/
ECL_EXPORTED_API void ECL_Huff8_CompressCTree768(const uint8_t* ctree768, uint16_t n_unique, uint8_t* depth_buf_x2, uint16_t depth_buf_size, ECL_HUFF8_WSTREAM_Type* wstream);

// compresses to wstream only data itself, returns amount of bits written (or 0 in case of error)
ECL_EXPORTED_API uint32_t ECL_Huff8_CompressDataWithTSpec1024(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, const uint32_t* tspec1024/*[256]*/, ECL_HUFF8_WSTREAM_Type* wstream);

/*
    TODO description
    returns amount of bits written (or 0 in case of error).
*/
ECL_EXPORTED_API uint32_t ECL_Huff8_Compress16_ULM(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint32_t* buf1024/*[256]*/, uint8_t* buf256, uint8_t* buf768, ECL_HUFF8_WSTREAM_Type* wstream);

/*
    TODO description
*/
#ifdef ECL_HUFF8_WSTREAM_Init
ECL_EXPORTED_API uint32_t ECL_Huff8_Compress16_ULM_Raw(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint32_t* buf1024/*[256]*/, uint8_t* buf256, uint8_t* buf768, uint8_t* dst, ECL_usize dst_size);
#endif



/* Decompress* user methods ------------------------------------------------------------------------------------------------------------------------ */

/*
    TODO description
*/
ECL_EXPORTED_API void ECL_Huff8_DecompressDTree1025(ECL_HUFF8_RSTREAM_Type* rstream, uint8_t* dst_dtree1025);

/*
    TODO description
*/
ECL_EXPORTED_API ECL_usize ECL_Huff8_DecompressWithDTree1025(const uint8_t* dtree1025, ECL_HUFF8_RSTREAM_Type* rstream, uint8_t* dst, ECL_usize bytes_cnt, ECL_usize interval);

/*
    TODO description
*/
ECL_EXPORTED_API ECL_usize ECL_Huff8_Decompress(ECL_HUFF8_RSTREAM_Type* rstream, uint8_t* dtree_buf/*1025*/, uint8_t* dst, ECL_usize bytes_cnt, ECL_usize interval);

/*
    TODO description
*/
#ifdef ECL_HUFF8_RSTREAM_Init
ECL_EXPORTED_API ECL_usize ECL_Huff8_Decompress_Raw(const uint8_t* src, ECL_usize src_size, uint8_t* dtree_buf/*1025*/, uint8_t* dst, ECL_usize bytes_cnt, ECL_usize interval);
#endif

#ifdef __cplusplus
}
#endif

#endif