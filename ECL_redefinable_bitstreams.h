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

#ifndef ECL_REDEFINABLE_BITSTREAMS_
#define ECL_REDEFINABLE_BITSTREAMS_

#include "ECL_JH_States.h"

/* input(R*) and output(W*) bitstreams and functions are used thru defines so they can be overridden if different work with memory is needed */
/* ECL_RSTREAM_Type and ECL_WSTREAM_Type are used in some of compressors - ones that explicitly include ECL_redefinable_bitstreams.h header */


#ifndef ECL_RSTREAM_Type /* if not defined by user - use standard one */
    #define ECL_RSTREAM_Type ECL_JH_RState
    #define ECL_RSTREAM_Read1_8 ECL_JH_Read /* ECL_RSTREAM_Read1_8(ECL_RSTREAM_Type, n_bits) where n_bits is 1..8 */

    /* optional functionality: use init and few more ECL_JH_RState specific hardcoded operations for respective *_Raw functions, which are otherwise disabled */
    /* so defining ECL_RSTREAM_JHx_Init states that RSTREAM is JH-compatible for use with provided Init function (e.g. could be an extention with several init* functions) */
    #define ECL_RSTREAM_JHx_Init(rstream, src_uint8_const_ptr, src_size_bytes) ECL_JH_RInit(rstream, src_uint8_const_ptr, src_size_bytes, 0)

    /* optional functionality for optimizations: RSTREAM:PeekWithinByte - peeks next data portion without reading - see ECL_JH_PeekWithinByte description */
    #define ECL_RSTREAM_PeekWithinByte(rstream, out_bits_ptr) ECL_JH_PeekWithinByte(rstream, out_bits_ptr)

#endif /* ECL_RSTREAM_Type */


#ifndef ECL_WSTREAM_Type /* if not defined by user - use standard one */
    #define ECL_WSTREAM_Type ECL_JH_WState
    #define ECL_WSTREAM_Write1_8 ECL_JH_Write /* ECL_WSTREAM_Write1_8(ECL_WSTREAM_Type, value, n_bits) where n_bits is 1..8 */

    /* optional functionality: use init and few more ECL_JH_WState specific hardcoded operations for respective *_Raw functions, which are otherwise disabled */
    /* so defining ECL_WSTREAM_JHx_Init states that WSTREAM is JH-compatible use with provided Init function (e.g. could be an extention with several init* functions) */
    #define ECL_WSTREAM_JHx_Init(wstream, dst_uint8_ptr, dst_size_bytes) ECL_JH_WInit(wstream, dst_uint8_ptr, dst_size_bytes, 0)

#endif /* ECL_WSTREAM_Type */


#endif /* ECL_REDEFINABLE_BITSTREAMS_ */
