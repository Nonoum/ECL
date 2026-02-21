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


#define ECL_SLA_FLAWLESS_HEADER 0x3F

#ifdef __cplusplus
extern "C" {
#endif

/*	'ECL_SLA_Analyze' generates optimal valid header of (src) with (size) samples.
	returns best FULL-bit count for compressed block (including bits needed to store the header),
	and writes header signature in (*dst_header) containing 1 byte.
*/
ECL_usize ECL_SLA_Analyze(const uint8_t* src, ECL_usize size, char* dst_header);

/*	'ECL_SLA_Compress' encodes (size) samples from (src) to (wstream) using (pre_calc_header) that was created before.
	if (pre_calc_header) is wrong then stream can DAMAGE information (not verified for now).
	stream DOESN'T contain information about size of input or output data.
    returns 1 in case of success, 0 in case of error (NULL pointers etc).
*/
uint8_t ECL_SLA_Compress(const uint8_t* src, ECL_usize size, char pre_calc_header, ECL_WSTREAM_Type* wstream);

/*	'ECL_SLA_Decompress' decodes data from (rstream) to (dst) expecting that there is (size) of encoded samples.
    returns 1 in case of success, 0 in case of error (NULL pointers etc).
*/
uint8_t ECL_SLA_Decompress(ECL_RSTREAM_Type* rstream, uint8_t* dst, ECL_usize size);



#ifdef ECL_WSTREAM_JHx_Init /* *Raw functions require ECL_WSTREAM_JHx_Init, which is present by default, but needs to be defined if custom ECL_WSTREAM_Type is chosen */
uint8_t ECL_SLA_Compress_Raw(const uint8_t* src, ECL_usize src_size, char pre_calc_header, uint8_t* dst, ECL_usize dst_size);
#endif /* ECL_WSTREAM_JHx_Init */


#ifdef ECL_RSTREAM_JHx_Init
uint8_t ECL_SLA_Decompress_Raw(const uint8_t* src, ECL_usize src_size, uint8_t* dst, ECL_usize size);
#endif /* ECL_RSTREAM_JHx_Init */


#ifdef __cplusplus
}
#endif

#endif /* ECL_SLA_ */
