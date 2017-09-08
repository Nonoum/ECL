#ifndef ECL_CONFIG_H_
#define ECL_CONFIG_H_

#include <assert.h>
#include <stdint.h>

// user setup part ---------------------------------------------
/*
    Add your own difinitions here if you don't have available:
    - uint8_t
    - uint16_t
    - uint32_t
*/

#ifndef ECL_USE_BITNESS_16
#ifndef ECL_USE_BITNESS_32
#ifndef ECL_USE_BITNESS_64
// none of these is defined with force for compilation, use 32 bits by default
#define ECL_USE_BITNESS_32 // default bitness
#endif
#endif
#endif

//#define ECL_USE_ASSERT
//#define ECL_USE_BRANCHLESS



// size types ---------------------------------------------
#ifdef ECL_USE_BITNESS_16
typedef uint16_t ECL_usize;
typedef int16_t ECL_ssize;
#define ECL_SIZE_TYPE_BITS_COUNT 16

#elif defined ECL_USE_BITNESS_32
typedef uint32_t ECL_usize;
typedef int32_t ECL_ssize;
#define ECL_SIZE_TYPE_BITS_COUNT 32

#elif defined ECL_USE_BITNESS_64
typedef uint64_t ECL_usize;
typedef int64_t ECL_ssize;
#define ECL_SIZE_TYPE_BITS_COUNT 64

#endif

#define ECL_POINTER_BITS_COUNT (sizeof(void*) * 8)



// asserts ---------------------------------------------
#ifdef ECL_USE_ASSERT
#define ECL_STATIC_ASSERT(expr, msg) static_assert((expr), msg)
#define ECL_ASSERT(expr) assert(expr)
#else
#define ECL_STATIC_ASSERT(expr, msg)
#define ECL_ASSERT(expr)
#endif

#endif
