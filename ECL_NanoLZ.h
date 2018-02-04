#ifndef ECL_NANO_LZ_
#define ECL_NANO_LZ_

#include "ECL_config.h"

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
#define ECL_NANO_LZ_GET_FAST1_MAP_BUF_SIZE() (256 * sizeof(ECL_usize))
#define ECL_NANO_LZ_GET_FAST1_WINDOW_BUF_SIZE(window_size_bits) ((1UL << (window_size_bits)) * sizeof(ECL_usize))
//
#define ECL_NANO_LZ_GET_FAST2_MAP_BUF_SIZE() (65536 * sizeof(ECL_usize))
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
    - not included as identifiers in compressed stream in any way (so decompression method has such parameter as well);
    - intended to be tested with target datasets to determine optimal one (different schemes would provide different compression level);
    - can be extended with your own scheme, optimized for your particular data (if you know what you're doing);
*/
typedef enum {
    ECL_NANOLZ_SCHEME1,
} ECL_NanoLZ_Scheme;

#define ECL_NANO_LZ_SCHEMES_ALL {ECL_NANOLZ_SCHEME1}

/*
    Compresses 'src_size' bytes starting at 'src' to destination 'dst' that can hold at most 'dst_size' bytes.
    Function returns amount of bytes in resulted compressed stream or 0 if failed.
    Extra memory usage: 0.
    - 'search_limit' is maximum amount of bytes to look back when searching for a match, increasing this parameter can decrease performance dramatically.
    - to find enough size for output buffer: dst_size = ECL_NANO_LZ_GET_BOUND(src_size);
    See full compress/decompress example usage near decompression function.
*/
ECL_usize ECL_NanoLZ_Compress_slow(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit);


// TODO docs
ECL_usize ECL_NanoLZ_Compress_mid1(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit, void* buf_256);
ECL_usize ECL_NanoLZ_Compress_mid2(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit, void* buf_512);
ECL_usize ECL_NanoLZ_Compress_mid1min(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, void* buf_512);
ECL_usize ECL_NanoLZ_Compress_mid2min(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, void* buf_512);


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
    - consumption with alloc1 is 256*sizeof(ECL_usize) + (1 << window_size_bits)*sizeof(ECL_usize)
    - consumption with alloc2 is 65536*sizeof(ECL_usize) + (1 << window_size_bits)*sizeof(ECL_usize)

    Initializers for fast1/fast2 compressors, return whether succeeded.
    - *Alloc1 allocates buffers with sizes according to ECL_NANO_LZ_GET_FAST1* macros
    - *Alloc2 allocates buffers with sizes according to ECL_NANO_LZ_GET_FAST2* macros

    For maximum efficiency window_size_bits should be = [log2(src_size)].
*/
bool ECL_NanoLZ_FastParams_Alloc1(ECL_NanoLZ_FastParams* p, uint8_t window_size_bits);
bool ECL_NanoLZ_FastParams_Alloc2(ECL_NanoLZ_FastParams* p, uint8_t window_size_bits);

/*
    Deallocates buffers - call after you're done with FastParams.
*/
void ECL_NanoLZ_FastParams_Destroy(ECL_NanoLZ_FastParams* p);

/*
    Indexed (optimized) compressors using pre-allocated buffers, behave equally to ECL_NanoLZ_Compress_slow with only difference in speed and used memory.
    - prefer fast1 algorithm for small data sets (up to hundreds of bytes)
    - prefer fast2 for big datasets (thousands of bytes and more)

    Memory consumption of these methods is big for embedded but it would work well if you compress data on powerful (say, PC) side and send it
    to embedded hardware where it's decompressed. To Compress data on hardware with limited resources you would sacrifice either compression ratio or time.

    Normal 'search_limit' value is typically around 10 - 50, for small datasets don't hesitate to set to -1.

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
    Decompresses exactly 'dst_size' bytes to 'dst' from compressed 'src' stream containing 'src_size' bytes.
    Function returns: amount of bytes in uncompressed stream, which is equal to 'dst_size' if decompression succeeded.
    Extra memory usage: 0.
    Usage:
        MyPODDataStruct my_data;
        ECL_usize compressed_size_limit = ECL_NANO_LZ_GET_BOUND(sizeof(my_data));
        uint8_t* compressed_stream = (uint8_t*)malloc( compressed_size_limit );
        ECL_usize compressed_size = ECL_NanoLZ_Compress_slow(ECL_NANOLZ_SCHEME1, (const uint8_t*)&my_data, sizeof(my_data), compressed_stream, compressed_size_limit, 1000);
        // ... <- transferring 'compressed_size' bytes of 'compressed_stream' to receiver side
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
