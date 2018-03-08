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

#ifndef ECL_NANO_LZ_
#define ECL_NANO_LZ_

#include "ECL_config.h"
#include "ECL_utils.h"

#include <stdbool.h>

/*
    Calculates size of buffer to fit compressed version of any data of 'src_size' size.
    Formula is for ECL_NANOLZ_SCHEME1 scheme, for other schemes may not give enough size,
    but in such case when compression fails - size is already bigger than source size, so compressed stream isn't worth to be used.
*/
#define ECL_NANO_LZ_GET_BOUND(src_size) ((src_size) + 2 + sizeof(ECL_usize) + (sizeof(ECL_usize) / 4))

/*
    Following macros are for manual allocations of buffers for fast1/fast2 algorithms.
*/
#define ECL_NANO_LZ_GET_FAST1_MAP_BUF_SIZE() (257 * sizeof(ECL_usize))
#define ECL_NANO_LZ_GET_FAST1_WINDOW_BUF_SIZE(window_size_bits) (((1UL << (window_size_bits)) + 1) * sizeof(ECL_usize))
/* */
#define ECL_NANO_LZ_GET_FAST2_MAP_BUF_SIZE() (65537 * sizeof(ECL_usize))
#define ECL_NANO_LZ_GET_FAST2_WINDOW_BUF_SIZE(window_size_bits) ECL_NANO_LZ_GET_FAST1_WINDOW_BUF_SIZE(window_size_bits)


#ifdef __cplusplus
extern "C" {
#endif

/*
    Declare static counters for tracking statistic on data decompression.
*/
#ifdef ECL_USE_STAT_COUNTERS
#define ECL_NANO_LZ_DECOMPRESSION_OPCODE_PICK_COUNTERS_COUNT (8+1)
extern int ECL_NanoLZ_Decompression_OpcodePickCounters[ECL_NANO_LZ_DECOMPRESSION_OPCODE_PICK_COUNTERS_COUNT];
#endif


/*
    Schemes are:
    - incompatible between each other, so you have to use same scheme during compression and decompression of your dataset for correct result;
    - not included as identifiers in compressed stream in any way (so decompression method has 'scheme' parameter as well);
    - intended to be tested with target datasets to determine optimal one (different schemes would provide different compression level and performance);
    - can be extended with your own scheme, optimized for your particular data (if you know what you're doing),
        in this case it's recommended to assign bigger values for them (e.g. ECL_NANOLZ_PETER_SCHEME = 128,)
        in case you plan to use ECL as dynamic library and need binary compatibility.
*/
typedef enum {
#if ECL_NANO_LZ_IS_SCHEME_ENABLED(1)
    ECL_NANOLZ_SCHEME1 = 0, /* main scheme, highly optimized for small datasets */
#endif
#if ECL_NANO_LZ_IS_SCHEME_ENABLED(2)
    ECL_NANOLZ_SCHEME2_DEMO = 1, /* demo scheme. small code size, weak compression, high decompression speed - example for writing custom scheme */
#endif
    /* user-defined scheme codes are recommended to start from 128 */
} ECL_NanoLZ_Scheme;


/* macro representing set of all schemes (used for testing) */
#if (ECL_NANO_LZ_ONLY_SCHEME == 0)
    #define ECL_NANO_LZ_SCHEMES_ALL {ECL_NANOLZ_SCHEME1, ECL_NANOLZ_SCHEME2_DEMO}
#elif (ECL_NANO_LZ_ONLY_SCHEME == 1)
    #define ECL_NANO_LZ_SCHEMES_ALL {ECL_NANOLZ_SCHEME1}
#else
    #define ECL_NANO_LZ_SCHEMES_ALL {ECL_NANOLZ_SCHEME2_DEMO}
#endif


/*
    The slowest mode, compresses 'src_size' bytes starting at 'src' to destination 'dst' that can hold at most 'dst_size' bytes.
    Function returns amount of bytes in resulted compressed stream, or 0 in case of error.
    Extra memory usage: 0.
    - 'search_limit' is maximum amount of bytes to look back when searching for a match, increasing this parameter can decrease performance dramatically.
    - to find enough size for output buffer: dst_size = ECL_NANO_LZ_GET_BOUND(src_size);
    See full compress/decompress example usage near decompression function.
*/
ECL_usize ECL_NanoLZ_Compress_slow(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit);


/*
    All mid* compressors are the most applicable for embedded. Require temporary buffer (not intersecting src or dst) provided via last parameter:
    - mid1, mid1min have 'buf_256' parameter which has to point to a buffer of 256 bytes;
    - mid2, mid2min have 'buf_513' (NOTE: not 512) parameter which has to point to a buffer of 513 bytes;

    All mid* algorithms require 'src_size' to not exceed 64k.

    In particular mid1, mid2 algorithms require 'dst_size' to be bigger than 'src_size'/2 (in case you planned to neglect ECL_NANO_LZ_GET_BOUND macro),
    also if your data is barely compressible - these two algorithms will work better if you give 'dst_size' = up to "ECL_NANO_LZ_GET_BOUND(src_size) + src_size/2 + 1"
    (see ECL_NANO_LZ_MID_GET_BOUND_OPTIMAL).

    mid1min, mid2min are optimized versions of mid1, mid2 with 'search_limit'==1. Provide higher performance and smaller code size (but only basic compression),
    these are by fact the fastest NanoLZ compression methods.
*/
ECL_usize ECL_NanoLZ_Compress_mid1(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit, void* buf_256);
ECL_usize ECL_NanoLZ_Compress_mid2(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit, void* buf_513);
ECL_usize ECL_NanoLZ_Compress_mid1min(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, void* buf_256);
ECL_usize ECL_NanoLZ_Compress_mid2min(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, void* buf_513);

/*
    Helper macro defining optimal reserved output size for mid* algorithms (except mid*min)
    - reserving extra space in output can improve compression ratio of mid* algorithms in cases when data is poorly compressible.
*/
#define ECL_NANO_LZ_MID_GET_BOUND_OPTIMAL(src_size) (ECL_NANO_LZ_GET_BOUND(src_size) + ((src_size)/2) + 1)


/*
    Struct with parameters to call fast1/fast2 compression algorithms, has to be initialized with
    ECL_NanoLZ_FastParams_Alloc1/ECL_NanoLZ_FastParams_Alloc2 respectively and destroyed with ECL_NanoLZ_FastParams_Destroy.
*/
typedef struct {
    void* buf_map;
    void* buf_window;
    uint8_t window_size_bits;
} ECL_NanoLZ_FastParams;

/*
    TLDR version:
    - consumption with *Alloc1 is (258 + (1 << window_size_bits)) * sizeof(ECL_usize)
    - consumption with *Alloc2 is (65538 + (1 << window_size_bits)) * sizeof(ECL_usize)

    Initializers for fast1/fast2 compressors, return whether succeeded.
    - *Alloc1 allocates buffers with sizes according to ECL_NANO_LZ_GET_FAST1* macros - valid for fast1 algorithm;
    - *Alloc2 allocates buffers with sizes according to ECL_NANO_LZ_GET_FAST2* macros - valid for fast2 and fast1 algorithms;

    For maximum efficiency window_size_bits should be = [log2(src_size)] (see ECL_LogRoundUp function).
*/
bool ECL_NanoLZ_FastParams_Alloc1(ECL_NanoLZ_FastParams* p, uint8_t window_size_bits);
bool ECL_NanoLZ_FastParams_Alloc2(ECL_NanoLZ_FastParams* p, uint8_t window_size_bits);

/*
    Deallocates buffers - call after you're done with FastParams. Necessary to call if *Alloc succeeded.
*/
void ECL_NanoLZ_FastParams_Destroy(ECL_NanoLZ_FastParams* p);

/*
    Faster indexed (optimized) compressors using pre-allocated buffers, behave equally to ECL_NanoLZ_Compress_slow with only difference in speed and used memory.
    - prefer fast1 algorithm for small datasets (roughly, < 1500 bytes);
    - prefer fast2 for bigger datasets;

    Memory consumption of these methods is big for embedded but it would work well if you compress data on powerful (say, PC) side and send it
    to embedded hardware where it's decompressed. To compress data on hardware with limited resources you would sacrifice either compression ratio or time.

    Normal 'search_limit' value is typically around 10 - 50. Value of '-1' aims to maximum (and longest) compression - for small datasets should be fine.

    Usage:
        MyPODDataStruct my_data;
        ECL_usize compressed_size_limit = ECL_NANO_LZ_GET_BOUND(sizeof(my_data));
        uint8_t* compressed_stream = (uint8_t*)malloc( compressed_size_limit );
        ECL_NanoLZ_FastParams fp;
        ECL_NanoLZ_FastParams_Alloc2(&fp, 10);
        ECL_usize compressed_size = ECL_NanoLZ_Compress_fast2(ECL_NANOLZ_SCHEME1, (const uint8_t*)&my_data, sizeof(my_data), compressed_stream, compressed_size_limit, 20, &fp);
        ECL_NanoLZ_FastParams_Destroy(&fp);
*/
ECL_usize ECL_NanoLZ_Compress_fast1(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit, ECL_NanoLZ_FastParams* p);
ECL_usize ECL_NanoLZ_Compress_fast2(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit, ECL_NanoLZ_FastParams* p);

/*
    Generic compression method for non-limited environment (not embedded).
    Calls one of other compression methods (and allocates appropriate buffers according to input data size), to result in maximum performance.
*/
ECL_usize ECL_NanoLZ_Compress_auto(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit);
/*
    Extended _auto version - allows to specify successfully preallocated FastParams with *Alloc1 and *Alloc2 (prealloc1 and prealloc2 correspondingly).
    If a preallocated parameter is NULL - default allocation occurs inside.
    Allows:
    - to store successfully preallocated FastParams outside and use them for multiple compression runs, thus saving time on malloc/free calls;
    - to specify custom window size.
*/
ECL_usize ECL_NanoLZ_Compress_auto_ex(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit
                                      , ECL_NanoLZ_FastParams* prealloc1, ECL_NanoLZ_FastParams* prealloc2);

/*
    Decompresses exactly 'dst_size' bytes to 'dst' from compressed 'src' stream containing 'src_size' bytes.
    Function returns: amount of bytes in uncompressed stream (which is 'dst_size') if decompression succeeded, or 0 in case of error.
    Extra memory usage: 0.
    Usage:
        MyPODDataStruct my_data;
        ECL_usize compressed_size_limit = ECL_NANO_LZ_GET_BOUND(sizeof(my_data));
        uint8_t* compressed_stream = (uint8_t*)malloc( compressed_size_limit );
        ECL_usize compressed_size = ECL_NanoLZ_Compress_slow(ECL_NANOLZ_SCHEME1, (const uint8_t*)&my_data, sizeof(my_data), compressed_stream, compressed_size_limit, 30);
        // <- transferring 'compressed_size' bytes of 'compressed_stream' to receiver side
        ECL_usize uncompressed_size = ECL_NanoLZ_Decompress(ECL_NANOLZ_SCHEME1, compressed_stream, compressed_size, (uint8_t*)&my_data, sizeof(my_data));
        if(uncompressed_size != sizeof(my_data)) {
            // failed
        }
*/
ECL_usize ECL_NanoLZ_Decompress(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size);

#ifdef __cplusplus
}
#endif

#endif
