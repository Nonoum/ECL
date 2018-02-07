#ifndef ECL_CONFIG_H_
#define ECL_CONFIG_H_

#include <stdint.h>
#include <stdlib.h> // alloc
#include <string.h> // memcpy, memset

#define ECL_VERSION_MAJOR 0
#define ECL_VERSION_MINOR 1
#define ECL_VERSION_PATCH 0
#define ECL_VERSION_BRANCH 'm'


// user setup part ---------------------------------------------
/*
    Add your own definitions here if you don't have available:
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
//#define ECL_USE_STAT_COUNTERS



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

/*
    Helper paranoid macro to ensure that estimated compression bound isn't overflowed.
    Results in bool. Makes sense for ECL_USE_BITNESS_16.
    Usage: assert( ECL_VALIDATE_BOUND_OK( ECL_NANO_LZ_GET_BOUND(src_size), src_size ) );
*/
#define ECL_VALIDATE_BOUND_OK(estimated_max_compressed_size, src_size) (( (ECL_usize)(estimated_max_compressed_size) ) > ( (ECL_usize)(src_size) ))


// asserts ---------------------------------------------
#ifdef ECL_USE_ASSERT
    #include <assert.h>
    #define ECL_STATIC_ASSERT(expr, msg) static_assert((expr), msg)
    #define ECL_ASSERT(expr) assert(expr)
#else
    #define ECL_STATIC_ASSERT(expr, msg)
    #define ECL_ASSERT(expr)
#endif



// helpful
#define ECL_MIN(a, b) ((a) < (b) ? (a) : (b))
#define ECL_MAX(a, b) ((a) > (b) ? (a) : (b))

#endif
