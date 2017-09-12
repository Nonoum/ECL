#ifndef ECL_ZERO_EATER_H_
#define ECL_ZERO_EATER_H_

#include "ECL_config.h"

#define ECL_ZERO_EATER_GET_BOUND(src_size) ((src_size) + 1 + (src_size)/128)

#ifdef __cplusplus
extern "C" {
#endif

/*
    Compresses 'src_size' bytes starting at 'src' to destination 'dst' that can hold at most 'dst_size' bytes.
    Function returns amount of bytes in resulted compressed stream or 0 if parameters are invalid.
    'dst' is allowed to be NULL for dry run, if 'dst' is not NULL and 'dst_size' is not less than returned value, then 'dst' contains valid output.
    To find enough size for output buffer you can:
    - use dry run: dst_size = ECL_ZeroEater_Compress(src, src_size, 0, 0);
    - use maximum size: dst_size = ECL_ZERO_EATER_GET_BOUND(src_size);
*/
ECL_usize ECL_ZeroEater_Compress(const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size);

/*
    Decompresses exactly 'src_size' bytes starting at 'src' to destination 'dst' holding at most 'dst_size' bytes.
    Function returns: amount of bytes in uncompressed stream, or 0 in case of error.
    Usage:
        uint8_t* cmp = (uint8_t*)malloc( ECL_ZERO_EATER_GET_BOUND(sizeof(my_data)) );
        ECL_usize compressed_size = ECL_ZeroEater_Compress(src, src_size, cmp, compressed_size);
        // ... transferring 'compressed_size' bytes of 'cmp' to receiver side
        ECL_usize uncompressed_size = ECL_ZeroEater_Decompress(cmp, compressed_size, (uint8_t*)&my_data, sizeof(my_data));
        if(uncompressed_size != sizeof(my_data)) {
            // failed
        }
*/
ECL_usize ECL_ZeroEater_Decompress(const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize dst_size);

#ifdef __cplusplus
}
#endif

#endif
