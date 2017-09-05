#ifndef ECL_NANO_LZ_
#define ECL_NANO_LZ_

#include "ECL_config.h"

/*
    Calculates size of buffer to fit compressed version of any data of 'src_size' size.
*/
#define ECL_NANO_LZ_GET_BOUND(src_size) (((src_size) * 2) + 1000) // TODO testing version
//#define ECL_NANO_LZ_GET_BOUND(src_size) ((src_size) + 2 + sizeof(ECL_usize) + (sizeof(ECL_usize) / 4))

/*
TODO
    Compresses 'src_size' bytes starting at 'src' to destination 'dst' that can hold at most 'dst_size' bytes.
    Function returns amount of bytes in resulted compressed stream or 0 if failed.
    To find enough size for output buffer:
    - use maximum size: dst_size = ECL_NANO_LZ_GET_BOUND(src_size);
*/
ECL_usize ECL_NanoLZ_Compress(const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size);

/*
TODO
    Decompresses exactly 'src_size' bytes starting at 'src' to destination 'dst' holding at most 'dst_size' bytes.
    Function returns: amount of bytes in uncompressed stream.
    Usage:
        uint8_t* cmp = (uint8_t*)malloc( ECL_NANO_LZ_GET_BOUND(sizeof(my_data)) );
        ECL_usize compressed_size = ECL_ZeroDevourer_Compress(src, src_size, cmp, compressed_size);
        // ... transferring 'compressed_size' bytes of 'cmp' to receiver side
        ECL_usize uncompressed_size = ECL_ZeroDevourer_Decompress(cmp, compressed_size, (uint8_t*)&my_data, sizeof(my_data));
        if(uncompressed_size != sizeof(my_data)) {
            // failed
        }
*/
ECL_usize ECL_NanoLZ_Decompress(const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size);

// TODO
//#define ECL_ZERO_EATER_GET_BOUND ECL_NANO_LZ_GET_BOUND
//#define ECL_ZeroEater_Compress ECL_NanoLZ_Compress
//#define ECL_ZeroEater_Decompress ECL_NanoLZ_Decompress

#endif
