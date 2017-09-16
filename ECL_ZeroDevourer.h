#ifndef ECL_ZERO_DEVOURER_H_
#define ECL_ZERO_DEVOURER_H_

#include "ECL_config.h"

/*
    Calculates size of buffer to fit compressed version of any data of 'src_size' size.
*/
#define ECL_ZERO_DEVOURER_GET_BOUND(src_size) ((src_size) + 4)

#ifdef __cplusplus
extern "C" {
#endif

/*
    Compresses 'src_size' bytes starting at 'src' to destination 'dst' that can hold at most 'dst_size' bytes.
    Function returns amount of bytes in resulted compressed stream or 0 if failed.
    To find enough size for output buffer:
    - use maximum size: dst_size = ECL_ZERO_DEVOURER_GET_BOUND(src_size);
*/
ECL_usize ECL_ZeroDevourer_Compress(const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size);

/*
    Decompresses exactly 'dst_size' bytes to 'dst' from compressed 'src' stream containing 'src_size' bytes.
    Function returns: amount of bytes in uncompressed stream, which is equal to 'dst_size' if decompression succeeded.
    Usage:
        uint8_t* cmp = (uint8_t*)malloc( ECL_ZERO_DEVOURER_GET_BOUND(sizeof(my_data)) );
        ECL_usize compressed_size = ECL_ZeroDevourer_Compress(src, src_size, cmp, compressed_size);
        // ... transferring 'compressed_size' bytes of 'cmp' to receiver side
        ECL_usize uncompressed_size = ECL_ZeroDevourer_Decompress(cmp, compressed_size, (uint8_t*)&my_data, sizeof(my_data));
        if(uncompressed_size != sizeof(my_data)) {
            // failed
        }
*/
ECL_usize ECL_ZeroDevourer_Decompress(const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size);

#ifdef __cplusplus
}
#endif

#endif
