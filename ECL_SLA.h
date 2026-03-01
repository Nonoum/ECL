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

#ifndef ECL_SLA_
#define ECL_SLA_

#include "ECL_config.h"
#include "ECL_redefinable_bitstreams.h"

#include <stdbool.h>


#define ECL_SLA_GET_BOUND(src_size) ((src_size) + 1) /* at most src_size + 6 bits */
#define ECL_SLA_FLAWLESS_HEADER 0x3F /* header that can safely encode any data (no compression though) */


/******************************* REDEFINABLE *******************************/

/* #define ECL_SLA_DISABLE_NULL_CHECKS */ /* can be defined to omit NULL checks, zero-size checks */

/***************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

/*
    'ECL_SLA_Analyze' generates optimal valid header of (src) with (size) samples.
    returns best FULL-bit count for compressed block (including bits needed to store the header),
    and writes header signature in (*dst_header) containing 1 byte.
*/
ECL_EXPORTED_API ECL_usize ECL_SLA_Analyze(const uint8_t* src, ECL_usize size, char* dst_header);

/*
    'ECL_SLA_Compress' encodes (size) samples from (src) to (wstream) using (pre_calc_header) that was created before.
    if (pre_calc_header) is wrong then stream can DAMAGE information (not verified for now).
    stream DOESN'T contain information about size of input or output data.
    returns 1 in case of success, 0 in case of error (NULL pointers etc).
*/
ECL_EXPORTED_API uint8_t ECL_SLA_Compress(const uint8_t* src, ECL_usize size, char pre_calc_header, ECL_WSTREAM_Type* wstream);

/*
    'ECL_SLA_Decompress' decodes data from (rstream) to (dst) expecting that there is (size) of encoded samples.
    returns 1 in case of success, 0 in case of error (NULL pointers etc).
*/
ECL_EXPORTED_API uint8_t ECL_SLA_Decompress(ECL_RSTREAM_Type* rstream, uint8_t* dst, ECL_usize size);



#ifdef ECL_WSTREAM_JHx_Init /* Compress*Raw functions require ECL_WSTREAM_JHx_Init, which is present by default, but needs to be defined if custom ECL_WSTREAM_Type is chosen */
ECL_EXPORTED_API uint8_t ECL_SLA_Compress_Raw(const uint8_t* src, ECL_usize src_size, char pre_calc_header, uint8_t* dst, ECL_usize dst_size);
#endif /* ECL_WSTREAM_JHx_Init */


#ifdef ECL_RSTREAM_JHx_Init /* Decompress*Raw functions require ECL_RSTREAM_JHx_Init, which is present by default, but needs to be defined if custom ECL_RSTREAM_Type is chosen */
/*
    returns amount of BYTES consumed from 'src' (which is <= 'src_size'), or 0 in case of any error.
*/
ECL_EXPORTED_API ECL_usize ECL_SLA_Decompress_Raw(const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize size);
#endif /* ECL_RSTREAM_JHx_Init */




/* --------------------------------------------------------------------------------------------------- */

/*
    ECL_SLA_Pack*X and ECL_SLA_Unpack*X are complementary methods to prepare buffers for compression (Pack) and (Unpack) after decompression.
    'buffers' contains modified data - in such a way that SLA is expected to be able to compress it.
    Methods target chart data (array of integer sensor readings, audio samples etc) and perform linear diff and rearrangement in certain way.

    'buffers' in preallocated buffer of size == n_values * sizeof(*src) == n_values*2 in case of uint16_t;
    After packing 'buffers' are recommended to be compressed as continuous series of byte buffers (e.g. 2 consequent buffers of 'n_values' size),
        e.g. resulting in 2 *Compress calls in this case.

    See usage in 'sample/sample_sla_compress_wav.cpp'

    For *Unpack calls with 'block_offset' != 0 it's expected that:
        - respective *Pack call was also made with 'block_offset' != 0
        - and dst[block_offset-1] has value that were at src[block_offset-1] when calling *Pack.
    As in such case they rely on history (use sliding window for processing);
    If you need to avoid such effect - pass (ptr + offset, 0) instead of (ptr, offset).

    Functions are defined for unsigned types, for similar signed types simple pointer casts can be used (e.g. (const uint16_t*)(my_const_int16_t_ptr)).

    Functions return 1 in case of success, 0 in case of bad parameters (NULL pointers, 0==n_values).
*/
ECL_EXPORTED_API uint8_t ECL_SLA_PackU16(const uint16_t* src, ECL_usize block_offset, uint16_t n_values, uint8_t* buffers);
ECL_EXPORTED_API uint8_t ECL_SLA_UnpackU16(const uint8_t* buffers, uint16_t* dst, ECL_usize block_offset, uint16_t n_values);

ECL_EXPORTED_API uint8_t ECL_SLA_PackU32(const uint32_t* src, ECL_usize block_offset, uint16_t n_values, uint8_t* buffers);
ECL_EXPORTED_API uint8_t ECL_SLA_UnpackU32(const uint8_t* buffers, uint32_t* dst, ECL_usize block_offset, uint16_t n_values);

/* 24bit version isn't implemented but it's easy to make one */


#ifdef __cplusplus
}
#endif

#endif /* ECL_SLA_ */
