#ifndef ECL_ZERO_DEVOURER_H_
#define ECL_ZERO_DEVOURER_H_

#include "ECL_config.h"

/*
    Calculates size of buffer to fit compressed version of any data of 'src_size' size.
*/
#define ECL_ZERO_DEVOURER_GET_BOUND(src_size) ((src_size) + 2 * sizeof(ECL_usize))

#ifdef __cplusplus
extern "C" {
#endif

/*
    Compresses 'src_size' bytes starting at 'src' to destination 'dst' that can hold at most 'dst_size' bytes.
    Function returns amount of bytes in resulted compressed stream, or 0 in case of error.
    To find enough size for output buffer:
    - use maximum size: dst_size = ECL_ZERO_DEVOURER_GET_BOUND(src_size);
    See full compress/decompress example usage near decompression function.
*/
ECL_EXPORTED_API ECL_usize ECL_ZeroDevourer_Compress(const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size);

/*
    Decompresses exactly 'dst_size' bytes to 'dst' from compressed 'src' stream containing 'src_size' bytes.
    Function returns: amount of bytes in uncompressed stream (which is 'dst_size') if decompression succeeded, or 0 in case of error.
    Usage:
        MyPODDataStruct my_data;
        const uint8_t* src = (const uint8_t*)&my_data;
        ECL_usize src_size = sizeof(my_data);
        ECL_usize compressed_size_limit = ECL_ZERO_DEVOURER_GET_BOUND(src_size);
        uint8_t* compressed = (uint8_t*)malloc( compressed_size_limit );
        ECL_usize compressed_size = ECL_ZeroDevourer_Compress(src, src_size, compressed, compressed_size_limit);
        // ... <- transferring 'compressed_size' bytes of 'compressed' to receiver side
        ECL_usize uncompressed_size = ECL_ZeroDevourer_Decompress(compressed, compressed_size, (uint8_t*)&my_data, sizeof(my_data));
        if(uncompressed_size != sizeof(my_data)) {
            // failed
        }
*/
ECL_EXPORTED_API ECL_usize ECL_ZeroDevourer_Decompress(const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size);

#ifdef __cplusplus
}
#endif

#endif
