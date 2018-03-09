#ifndef ECL_UTILS_H_
#define ECL_UTILS_H_

#include "ECL_config.h"

#ifdef __cplusplus
extern "C" {
#endif

ECL_EXPORTED_API uint32_t ECL_GetSizeBitness(void); /* returns ECL_SIZE_TYPE_BITS_COUNT */
ECL_EXPORTED_API uint32_t ECL_GetVersionNumber(void); /* returns ECL_VERSION_NUMBER */
ECL_EXPORTED_API const char* ECL_GetVersionString(void); /* returns "major.minor.patch" version string */
ECL_EXPORTED_API const char* ECL_GetVersionBranch(void); /* returns ECL_VERSION_BRANCH */

ECL_EXPORTED_API uint8_t ECL_LogRoundUp(ECL_usize value); /* returns [log2(value)]. for 0 returns 1 */
ECL_EXPORTED_API uint16_t* ECL_GetAlignedPointer2(uint8_t* ptr); /* returns aligned pointer (shifts ptr forward if needed) matching uint16_t alignment */
ECL_EXPORTED_API ECL_usize* ECL_GetAlignedPointerS(uint8_t* ptr); /* returns aligned pointer (shifts ptr forward if needed) matching ECL_usize alignment */

/*
    Exported helper functions for storing separate 'size' values in E7 format.
    Functions receive pointer to first byte of data (data_start), it's capacity (max_bytes) and value/output_value parameters
    to write/read it respectively.
    Return pointer to first unused byte, or NULL in case of any error.
*/
ECL_EXPORTED_API uint8_t* ECL_Helper_WriteE7(uint8_t* data_start, ECL_usize max_bytes, ECL_usize value);
ECL_EXPORTED_API const uint8_t* ECL_Helper_ReadE7(const uint8_t* data_start, ECL_usize max_bytes, ECL_usize* output_value);

#ifdef __cplusplus
}
#endif

#endif
