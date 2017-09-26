#ifndef ECL_NANO_LZ_
#define ECL_NANO_LZ_

#include "ECL_config.h"

/*
    Calculates size of buffer to fit compressed version of any data of 'src_size' size.
*/
#define ECL_NANO_LZ_GET_BOUND(src_size) ((src_size) + 2 + sizeof(ECL_usize) + (sizeof(ECL_usize) / 4))

#ifdef __cplusplus
extern "C" {
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
    ECL_NANOLZ_SCHEME2,
} ECL_NanoLZ_Scheme;

/*
    Compresses 'src_size' bytes starting at 'src' to destination 'dst' that can hold at most 'dst_size' bytes.
    Function returns amount of bytes in resulted compressed stream or 0 if failed.
    - 'search_limit' is maximum amount of bytes to look back when searching for a match, increasing this parameter can decrease performance dramatically.
    - to find enough size for output buffer: dst_size = ECL_NANO_LZ_GET_BOUND(src_size);
*/
ECL_usize ECL_NanoLZ_Compress_slow(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit);

typedef struct {
    void* buf_map;
    void* buf_window;
    uint8_t window_size_bits;
} ECL_NanoLZ_FastParams;

void ECL_NanoLZ_FastParams_Alloc1(ECL_NanoLZ_FastParams* p, uint8_t window_size_bits);
void ECL_NanoLZ_FastParams_Alloc2(ECL_NanoLZ_FastParams* p, uint8_t window_size_bits);
void ECL_NanoLZ_FastParams_Destroy(ECL_NanoLZ_FastParams* p);

ECL_usize ECL_NanoLZ_Compress_fast1(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit, ECL_NanoLZ_FastParams* p);
ECL_usize ECL_NanoLZ_Compress_fast2(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size, ECL_usize search_limit, ECL_NanoLZ_FastParams* p);

/*
    Decompresses exactly 'dst_size' bytes to 'dst' from compressed 'src' stream containing 'src_size' bytes.
    Function returns: amount of bytes in uncompressed stream, which is equal to 'dst_size' if decompression succeeded.
    Usage:
        uint8_t* cmp = (uint8_t*)malloc( ECL_NANO_LZ_GET_BOUND(sizeof(my_data)) );
        ECL_usize compressed_size = ECL_NanoLZ_Compress_slow(ECL_NANOLZ_SCHEME1, src, src_size, cmp, compressed_size);
        // ... transferring 'compressed_size' bytes of 'cmp' to receiver side
        ECL_usize uncompressed_size = ECL_NanoLZ_Decompress(ECL_NANOLZ_SCHEME1, cmp, compressed_size, (uint8_t*)&my_data, sizeof(my_data));
        if(uncompressed_size != sizeof(my_data)) {
            // failed
        }
*/
ECL_usize ECL_NanoLZ_Decompress(ECL_NanoLZ_Scheme scheme, const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size);

#ifdef __cplusplus
}
#endif

#endif
