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
#include "ECL_redefinable_bitstreams.h"

/******************************* CORE DEFINES *******************************/
#define ECL_HUFF8_MAX_UNIQUE_VALUES 256
#define ECL_HUFF8_MAX_TREE_NODES (2*ECL_HUFF8_MAX_UNIQUE_VALUES - 1)
#define ECL_HUFF8_COMPRESSED_TREE_SIZE_BITS(n_unique_bytes) ((8+2)*(n_unique_bytes) - 1)

/*
    Calculates size of buffer to fit compressed version of any data (along with tree) of 'src_size' size.
*/
#define ECL_HUFF8_GET_BOUND(src_size) ((src_size) + 1 + (ECL_HUFF8_COMPRESSED_TREE_SIZE_BITS(256) / 8))

/*
    Maximum Huffman tree depth supported by ULM implementation - covers any data up to 64k bytes (uint16_t size).
    Constant is used to allocate small buffers on stack (in decompressor - see it's documentation) or within existing supplied buffers for compressor (no extra stack arrays there).

    Maximum depth is maximum path from root to a leave, where direct "root->leave" depth=1.
    Maximum safe data size is input data size for which any data combination will result in a tree with depth not exceeding appropriate limit,
        although this is very edge case (e.g. for depth=12, max=841 - you can expect it to work ok for several kb data size or even more).
    Maximum safe data size per-depth is:
        for depth = 0 max = 1 (0x1) [special case - if depth is 0 - there's only one leaf (all values are equal) and no further compressed data stream]
        for depth = 1 max = 2 (0x2)
        for depth = 2 max = 5 (0x5)
        for depth = 3 max = 9 (0x9)
        for depth = 4 max = 16 (0x10)
        for depth = 5 max = 27 (0x1b)
        for depth = 6 max = 45 (0x2d)
        for depth = 7 max = 74 (0x4a)
        for depth = 8 max = 121 (0x79)
        for depth = 9 max = 197 (0xc5)
        for depth = 10 max = 320 (0x140)
        for depth = 11 max = 519 (0x207)
        for depth = 12 max = 841 (0x349) [12 is tspec512 maximum supported depth]
        for depth = 13 max = 1362 (0x552)
        for depth = 14 max = 2205 (0x89d)
        for depth = 15 max = 3569 (0xdf1)
        for depth = 16 max = 5776 (0x1690) [16 is tspec768 maximum supported depth]
        for depth = 17 max = 9347 (0x2483)
        for depth = 18 max = 15125 (0x3b15)
        for depth = 19 max = 24474 (0x5f9a)
        for depth = 20 max = 39601 (0x9ab1)
        for depth = 21 max = 64077 (0xfa4d)
        for depth = 22 max = 103680 (0x19500) [22 is chosen for ULM implementation to cover 64k (uint16_t size)]
        for depth = 23 max = 167759 (0x28f4f)
        for depth = 24 max = 271441 (0x42451) [24 is tspec1024 maximum supported depth]
        ...
        for depth = 32 max = 12752041 (0xc294a9)
        ...
        for depth = 50 max = 73681302245 (0x1127bf72e5)
        ...
        for depth = 56 max = 1322157322201 (0x133d6b7afd9)
        ...
        for depth = 64 max = 62113250390416 (0x387dde39b190)
*/
#define ECL_HUFF8_TREE_DEPTH_MAX_ULM 22

/* Maximum supported depths of tspec1024, tspec768, tspec512 formats respectively */
#define ECL_HUFF8_TREE_DEPTH_MAX_TSPEC1024 24
#define ECL_HUFF8_TREE_DEPTH_MAX_TSPEC768 16
#define ECL_HUFF8_TREE_DEPTH_MAX_TSPEC512 12



/******************************* REDEFINABLE *******************************/

/*
    Maximum Huffman tree depth supported by ECL_Huff8_Decompress* - length of uint16_t stack-allocated array within the Decompress function.
    Defaults to *_ULM (44 bytes buffer), you MIGHT want to reduce it for very restricted environment by providing the define earlier so it's not defaulted here
    (to choose value - see comments to ECL_HUFF8_TREE_DEPTH_MAX_ULM).
*/
#ifndef ECL_HUFF8_DECOMPRESS_MAX_DEPTH
    #define ECL_HUFF8_DECOMPRESS_MAX_DEPTH ECL_HUFF8_TREE_DEPTH_MAX_ULM
#endif

/* #define ECL_HUFF8_DISABLE_NULL_CHECKS */ /* can be defined to omit a lot of NULL checks, checks for zero size input and zero interval */

/* #define ECL_HUFF8_DISABLE_HSORT */ /* can be defined to reduce binary code size */

/* #define ECL_HUFF8_DECOMPRESS_CACHE_BITS_READING */ /* such define speeds up 'Decompress' if bits-reading functions are slow (e.g. for custom ECL_RSTREAM_Type) */


/***************************************************************************/


/*
    Buffers, formats and naming:
    - there are pointer parameters to arrays of various formats that have numbers in naming:
        - fooNNN where NNN is a number that represents amount of bytes, which is not necessary amount of elements in the array (if it's not uint8_t);
            - type of foo implies it's correct alignment (e.g. tspec1024==uint32_t[256] pointer implies it's aligned as uint32_t);
        - tspecNNN is a specification data used to compress user data stream (calculated for particular user data):
            - tspec1024 == uint32_t[256]  (1024/sizeof(uint32_t) == 256)
            - tspec768 == uint16_t[384]  (768/sizeof(uint16_t) == 384)
            - tspec512 == uint16_t[256]  (512/sizeof(uint16_t) == 256)
        - ctree768 is a huffman tree representation used for compress methods;
            - ctree768 == uint8_t[768];
        - dtree1024 is huffman decompressor tree representation (differs from ctree768);
            - dtree1024 == uint16_t[512]  (1024/sizeof(uint16_t) == 512)
        - bufNNN is some buffer needed for internal work (mostly used on high level / trivial use functions);

    Overall there's a lot of raw pointers work and splitting arrays to subarrays.
    There's intentionally no structs for related formats to minimize potential risks with alignment and allow simpler casting when advanced API is used.

    Many functions have several input buffers (in spite they could receive a single big one and split it internally) which results into a bit more cumbersome API
    - this is intentional as well, to allow more possibilities of buffers accomodation when RAM budget is very tight (e.g. to reuse whatever available space in particular point).

    Consider ECL_GetAlignedPointer* functions to ensure needed alignment if you're trying to reuse random RAM area (being used for other project purposes) as some Huff8 buffers.
    * [this can be redundant if your MCU is fine with unaligned memory access, but some Huff8 code can trigger ECL_ASSERTions for incorrect alignment].
*/

/*
    Global logic/flow (simplified WIKI for advanced use):
        Compression (simple):
            src data -> form 'freqs' -> form 'ctree' -> form 'spec' -> CompressCTree(ctree), CompressDataWith(src, spec);

        Compression (evaluated):
            src data -> form 'freqs' -> form 'ctree' -> form 'spec' -> evaluate output size using 'ctree' and 'spec' -> CompressCTree(ctree), CompressDataWith(src, spec);

        Decompression (simple):
            src compressed data -> decompress 'dtree' from src -> DecompressWithDTree(src, dtree)

        Decompression (predefined dtree):
            src compressed data -> DecompressWithDTree(src, dtree)

        Decompression (fast):
            src compressed data -> decompress 'dtree' from src -> form 'dtable768' using 'dtree' -> DecompressWithDTable768(src, dtree, dtable768)
*/

#ifdef __cplusplus
extern "C" {
#endif

/* Auxiliary methods (called internally, for advanced users) --------------------------------------------------------------------------------------- */

/*
    Iterates data from 'src' with interval of 'interval' bytes and amount of 'bytes_cnt' bytes to compress (e.g. src[interval*0], src[interval*1], src[interval*2], ... src[interval*(bytes_cnt-1)])
    to form output freqs == uint16_t[256] which is used for further work.
*/
ECL_EXPORTED_API void ECL_Huff8_FillFreqs16(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint16_t* freqs/*[256]*/);

/*
    Generates tree from freqs.
    'freqs' is input array uint16_t[256] with values [i] filled with counts of appearance of codes 'i' in input data.
    'buf256' is a 256-byte buffer to be supplied by user for internal needs.
    'out_ctree768' is two arrays in series:
        [{uint8_t left, uint8_t right}, ...] : 512 bytes; left/right are equally encoded and represent either an index of child node or value(code) depending of flags;
        [{uint8_t flags}, ...] : 256 bytes; indices match indices of records of previous array; if bit#0==1 then 'left' represents code, bit#1 is same for 'right';
        represents an encoded tree data needed for further processing, meaningful only if return value is > 1.
        Root code/record is top node, which has logical address of ('return value' - 2);
    returns amount of unique values (where freqs[i] are non-zero).
*/
ECL_EXPORTED_API uint16_t ECL_Huff8_Freqs16ToCTree768(uint16_t* freqs/*[256]*/, uint8_t* buf256, uint8_t* out_ctree768, uint16_t n_unique_max);



/*
    Generates spec from a tree in ctree768 format.
    'freqs' is input array uint16_t[256] with values [i] filled with counts off appearance of codes 'i' in input data.
    'out_tspec1024' is output spec data in tspec1024 format: 256 uint32_t values (aligned allocation), each value[i] representing info about how to encode [i]:
        - top 8 bits is uint8_t amount of bits in the bitcode;
        - lower 24 bits is bitcode;
    - 'depth_buf_x2' is external buffer needed for traversing the tree - it could be smaller or bigger depending on user needs:
        - for ULM 'depth_buf_x2' size is "2 * ECL_HUFF8_TREE_DEPTH_MAX_ULM"; ULM covers any user data up to 64k bytes (actually more).

    The function can't fail if 'ctree768' is result of ECL_Huff8_Freqs16ToCTree768 and 'n_unique' >= 2.
*/
ECL_EXPORTED_API void ECL_Huff8_CTree768ToTSpec1024_ULM(const uint8_t* ctree768, uint32_t* out_tspec1024/*[256]*/, uint8_t* depth_buf_x2);

/*
    TODO_BEFORE_HUFF8_RELEASE description

    Unlike 'ECL_Huff8_CTree768ToTSpec1024_ULM' the function can fail if tspec768 can't fit all needed codes due to depth limitation.
    returns 0 if failed, > 0 otherwise (may be defined some useful non-zero result in future).
*/
ECL_EXPORTED_API int16_t ECL_Huff8_CTree768ToTSpec768(const uint8_t* ctree768, uint16_t* out_tspec768/*[768/2 == 384]*/, uint8_t* depth_buf_x2);

/*
    TODO_BEFORE_HUFF8_RELEASE description

    Unlike 'ECL_Huff8_CTree768ToTSpec1024_ULM' the function can fail if tspec512 can't fit all needed codes due to depth limitation.
    returns 0 if failed, > 0 otherwise (may be defined some useful non-zero result in future).
*/
ECL_EXPORTED_API int16_t ECL_Huff8_CTree768ToTSpec512(const uint8_t* ctree768, uint16_t* out_tspec512/*[512/2 == 256]*/, uint8_t* depth_buf_x2);



/*
    Returns maximum tree depth (see ECL_HUFF8_TREE_DEPTH_* constants) used in the specific ctree768, requires "uint8_t buf512[512];" additional external buffer for calculation.
*/
ECL_EXPORTED_API uint16_t ECL_Huff8_GetMaxDepthCTree768(const uint8_t* ctree768, uint8_t* buf512);

/*
    Returns maximum tree depth (see ECL_HUFF8_TREE_DEPTH_* constants) used in the specific tspec1024.
*/
ECL_EXPORTED_API uint16_t ECL_Huff8_GetMaxDepthTSpec1024(const uint32_t* tspec1024/*[256]*/);

/*
    Returns maximum tree depth (see ECL_HUFF8_TREE_DEPTH_* constants) used in the specific tspec768.
*/
ECL_EXPORTED_API uint16_t ECL_Huff8_GetMaxDepthTSpec768(const uint16_t* tspec768/*[768/2 == 384]*/);

/*
    Returns maximum tree depth (see ECL_HUFF8_TREE_DEPTH_* constants) used in the specific tspec512.
*/
ECL_EXPORTED_API uint16_t ECL_Huff8_GetMaxDepthTSpec512(const uint16_t* tspec512/*[512/2 == 256]*/);





/* Evaluate/Analyze user methods - not modifying, estimate how much space is needed for compressed output ------------------------------------------ */

/* returns amount of bits needed to encode a tree for 'n_unique' unique elements ('n_unique' must be > 0) */
ECL_EXPORTED_API uint32_t ECL_Huff8_EvaluateTreeByN(uint16_t n_unique);

/* Same as ECL_Huff8_EvaluateTreeByN but retrieves 'n_unique' from the 'ctree768' */
ECL_EXPORTED_API uint32_t ECL_Huff8_EvaluateTree(const uint8_t* ctree768);

/*
    Effectively a dry-run of ECL_Huff8_CompressDataWithTSpec1024 (same as compression without saving output anywhere).
    returns amount of bits needed for compression of data stream (without tree), or 0 if can't compress it with given tspec1024.
*/
ECL_EXPORTED_API uint32_t ECL_Huff8_Evaluate16_ForTSpec1024(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, const uint32_t* tspec1024/*[256]*/);

/*
    Effectively a dry-run of ECL_Huff8_CompressDataWithTSpec768 (same as compression without saving output anywhere).
    returns amount of bits needed for compression of data stream (without tree), or 0 if can't compress it with given tspec768.
*/
ECL_EXPORTED_API uint32_t ECL_Huff8_Evaluate16_ForTSpec768(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, const uint16_t* tspec768/*[768/2 == 384]*/);

/*
    Effectively a dry-run of ECL_Huff8_CompressDataWithTSpec512 (same as compression without saving output anywhere).
    returns amount of bits needed for compression of data stream (without tree), or 0 if can't compress it with given tspec512.
*/
ECL_EXPORTED_API uint32_t ECL_Huff8_Evaluate16_ForTSpec512(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, const uint16_t* tspec512/*[512/2 == 256]*/);

/*
    Similar to ECL_Huff8_Compress16_ULM but doesn't write any encoded output, can be used to calculate required output buffer size.
    returns amount of bits needed for compression (or 0 in case of error).
*/
ECL_EXPORTED_API uint32_t ECL_Huff8_Analyze16_ULM(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint32_t* buf1024/*[256]*/, uint8_t* buf256, uint8_t* buf768);

/*
    Similar to ECL_Huff8_Analyze16_ULM but requires an extra buffer: buf512 is uint16_t[256] and works a bit faster.
*/
ECL_EXPORTED_API uint32_t ECL_Huff8_Analyze16_ULM_2k5(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint32_t* buf1024/*[256]*/, uint8_t* buf256, uint8_t* buf768, uint16_t* buf512);




/* Compress* user methods -------------------------------------------------------------------------------------------------------------------------- */

/*
    Compresses to wstream a tree in 'ctree768' format.
    TODO_BEFORE_HUFF8_RELEASE more description
    - 'depth_buf_x2' is external buffer needed for traversing the tree - it could be smaller or bigger depending on user needs:
        - for ULM 'depth_buf_x2' size: 2 * 'depth_buf_size' = 2 * ECL_HUFF8_TREE_DEPTH_MAX_ULM; ULM covers any user data up to 64k bytes (actually more).
        - for tiny datasets depth=8 could be enough ('depth_buf_size' = 8, depth_buf_x2 is 16 bytes long);
        - for any technically possible tree (possible on absurdly enormous data size) 'depth_buf_size' = 256, depth_buf_x2 is 512 bytes;
        - ! 'depth_buf_x2' buffer overflow isn't protected (with if/return), only checked with ECL_ASSERT;
*/
ECL_EXPORTED_API void ECL_Huff8_CompressCTree768(const uint8_t* ctree768, uint8_t* depth_buf_x2, uint16_t depth_buf_size, ECL_WSTREAM_Type* wstream);

/* compresses to wstream only data itself, returns amount of bits written (or 0 in case of error) */
ECL_EXPORTED_API uint32_t ECL_Huff8_CompressDataWithTSpec1024(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, const uint32_t* tspec1024/*[256]*/, ECL_WSTREAM_Type* wstream);

/* compresses to wstream only data itself, returns amount of bits written (or 0 in case of error) */
ECL_EXPORTED_API uint32_t ECL_Huff8_CompressDataWithTSpec768(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, const uint16_t* tspec768/*[768/2 == 384]*/, ECL_WSTREAM_Type* wstream);

/* compresses to wstream only data itself, returns amount of bits written (or 0 in case of error) */
ECL_EXPORTED_API uint32_t ECL_Huff8_CompressDataWithTSpec512(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, const uint16_t* tspec512/*[512/2 == 256]*/, ECL_WSTREAM_Type* wstream);

/*
    Compresses data from 'src' with interval of 'interval' bytes and amount of 'bytes_cnt' bytes to compress (e.g. src[interval*0], src[interval*1], src[interval*2], ... src[interval*(bytes_cnt-1)])
    to 'wstream' destination.
    Returns amount of BITS written to 'dst' in case of success, or 0 in case of error.
    The function can only fail if parameters are incorrect (e.g. NULL pointers) or dst_size is insufficient.
    Requires extra buffers for work:
        - buf1024 is uint32_t[256]; - uint32_t aligned
        - buf256 is uint8_t[256];
        - buf768 is uint8_t[768];
*/
ECL_EXPORTED_API uint32_t ECL_Huff8_Compress16_ULM(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint32_t* buf1024/*[256]*/, uint8_t* buf256, uint8_t* buf768, ECL_WSTREAM_Type* wstream);

/*
    Similar to ECL_Huff8_Compress16_ULM but:
        - has diffrent (smaller) extra buffers:
            - buf800 is uint16_t[400]; - uint16_t aligned
            - buf768 is uint8_t[768];
        - additionally can fail if:
            - 'bytes_cnt' > 5776 and src data has unlucky combination/statictics (roughly saying, the bigger 'bytes_cnt' - the bigger is such a chance).
*/
ECL_EXPORTED_API uint32_t ECL_Huff8_TryCompress16_TSpec768(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint16_t* buf800/*[800/2 == 400]*/, uint8_t* buf768, ECL_WSTREAM_Type* wstream);

/*
    Similar to ECL_Huff8_Compress16_ULM but:
        - has diffrent (smaller) extra buffers:
            - buf536 is uint16_t[268]; - uint16_t aligned
            - buf256 is uint8_t[256];
            - buf768 is uint8_t[768];
        - additionally can fail if:
            - 'bytes_cnt' > 841 and src data has unlucky combination/statictics (roughly saying, the bigger 'bytes_cnt' - the bigger is such a chance).
*/
ECL_EXPORTED_API uint32_t ECL_Huff8_TryCompress16_TSpec512(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint16_t* buf536/*[536/2 == 268]*/, uint8_t* buf256, uint8_t* buf768, ECL_WSTREAM_Type* wstream);


#ifdef ECL_WSTREAM_JHx_Init /* *Raw functions require ECL_WSTREAM_JHx_Init, which is present by default, but needs to be defined if custom ECL_WSTREAM_Type is chosen */

/*
    Compresses data from 'src' with interval of 'interval' bytes and amount of 'bytes_cnt' bytes to compress (e.g. src[interval*0], src[interval*1], src[interval*2], ... src[interval*(bytes_cnt-1)])
    to 'dst' that has 'dst_size' bytes capacity ('interval' doesn't apply to 'dst').
    Returns amount of BITS written to 'dst' in case of success, or 0 in case of error.
    The function can only fail if parameters are incorrect (e.g. NULL pointers) or dst_size is insufficient.
    Requires extra buffers for work:
        - buf1024 is uint32_t[256]; - uint32_t aligned
        - buf256 is uint8_t[256];
        - buf768 is uint8_t[768];
*/
ECL_EXPORTED_API uint32_t ECL_Huff8_Compress16_ULM_Raw(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint32_t* buf1024/*[256]*/, uint8_t* buf256, uint8_t* buf768, uint8_t* dst, ECL_usize dst_size);

/*
    Similar to ECL_Huff8_Compress16_ULM_Raw but:
        - has diffrent (smaller) extra buffers:
            - buf800 is uint16_t[400]; - uint16_t aligned
            - buf768 is uint8_t[768];
        - additionally can fail if:
            - 'bytes_cnt' > 5776 and src data has unlucky combination/statictics (roughly saying, the bigger 'bytes_cnt' - the bigger is such a chance).
*/
ECL_EXPORTED_API uint32_t ECL_Huff8_TryCompress16_TSpec768_Raw(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint16_t* buf800/*[800/2 == 400]*/, uint8_t* buf768, uint8_t* dst, ECL_usize dst_size);

/*
    Similar to ECL_Huff8_Compress16_ULM_Raw but:
        - has diffrent (smaller) extra buffers:
            - buf536 is uint16_t[268]; - uint16_t aligned
            - buf256 is uint8_t[256];
            - buf768 is uint8_t[768];
        - additionally can fail if:
            - 'bytes_cnt' > 841 and src data has unlucky combination/statictics (roughly saying, the bigger 'bytes_cnt' - the bigger is such a chance).
*/
ECL_EXPORTED_API uint32_t ECL_Huff8_TryCompress16_TSpec512_Raw(const uint8_t* src, uint16_t bytes_cnt, uint16_t interval, uint16_t* buf536/*[536/2 == 268]*/, uint8_t* buf256, uint8_t* buf768, uint8_t* dst, ECL_usize dst_size);

#endif





/* Decompress* user methods ------------------------------------------------------------------------------------------------------------------------ */

/*
    TODO_BEFORE_HUFF8_RELEASE description
*/
ECL_EXPORTED_API int16_t ECL_Huff8_DecompressDTree1024(ECL_RSTREAM_Type* rstream, uint16_t* dst_dtree1024/*[512]*/, uint16_t max_nodes/* <= 512*/);

/*
    TODO_BEFORE_HUFF8_RELEASE description
*/
ECL_EXPORTED_API ECL_usize ECL_Huff8_DecompressWithDTree1024(const uint16_t* dtree1024/*[512]*/, ECL_RSTREAM_Type* rstream, uint8_t* dst, ECL_usize bytes_cnt, ECL_usize interval);

/*
    TODO_BEFORE_HUFF8_RELEASE description

    returns TODO
*/
ECL_EXPORTED_API ECL_usize ECL_Huff8_Decompress(ECL_RSTREAM_Type* rstream, uint16_t* dtree_buf/*[512]*/, uint8_t* dst, ECL_usize bytes_cnt, ECL_usize interval);


/*
    Generates 'dtable768' special decompression cache (to be used along with dtree1024 in dedicated Decompress function).

    returns 1 in case of success;
    returns 0 if dtree1024 is empty;
    return -1 if dtree1024 consists of single element.
*/
ECL_EXPORTED_API int16_t ECL_Huff8_DTree1024ToDTable768(const uint16_t* dtree1024, uint16_t* dtable768/*[768/2 == 384]*/);

/*
    Runs decompression on 'rstream' source with prepared 'dtree1024' tree and 'dtable768' cache.

    Returns 0 in case of failure and 'bytes_cnt' in case of success.
*/
ECL_EXPORTED_API ECL_usize ECL_Huff8_DecompressWithDTable768(const uint16_t* dtree1024, const uint16_t* dtable768/*[768/2 == 384]*/, ECL_RSTREAM_Type* rstream, uint8_t* dst, ECL_usize bytes_cnt, ECL_usize interval);


#ifdef ECL_RSTREAM_JHx_Init

/*
    TODO_BEFORE_HUFF8_RELEASE description

    returns amount of bytes consumed from src (which is <= src_size), or 0 in case of any error.
*/
ECL_EXPORTED_API ECL_usize ECL_Huff8_Decompress_Raw(const uint8_t* src, ECL_usize src_size, uint16_t* dtree_buf/*[512]*/, uint8_t* dst, ECL_usize bytes_cnt, ECL_usize interval);

/*
    Similar to ECL_Huff8_Decompress_Raw but uses extra buffer for faster decompression.

    returns amount of bytes consumed from src (which is <= src_size), or 0 in case of any error.
*/
ECL_EXPORTED_API ECL_usize ECL_Huff8_DecompressWithDTable768_Raw(const uint8_t* src, ECL_usize src_size, uint16_t* dtree_buf/*[512]*/, uint16_t* dtable_buf/*[768/2 == 384]*/, uint8_t* dst, ECL_usize bytes_cnt, ECL_usize interval);

#endif /* ECL_RSTREAM_JHx_Init */


#ifdef __cplusplus
}
#endif

#endif /* ECL_HUFF8_ */
