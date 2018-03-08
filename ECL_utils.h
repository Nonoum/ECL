#ifndef ECL_UTILS_H_
#define ECL_UTILS_H_

#include "ECL_config.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8_t ECL_LogRoundUp(ECL_usize value); /* returns [log2(value)]. for 0 returns 1 */
uint16_t* ECL_GetAlignedPointer2(uint8_t* ptr); /* returns aligned pointer (shifts ptr forward if needed) matching uint16_t alignment */
ECL_usize* ECL_GetAlignedPointerS(uint8_t* ptr); /* returns aligned pointer (shifts ptr forward if needed) matching ECL_usize alignment */

#ifdef __cplusplus
}
#endif

#endif
