#ifndef ECL_CONFIG_H_
#define ECL_CONFIG_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h> /* memcpy, memset */

/* see runtime version in ECL_utils.h */
#define ECL_VERSION_MAJOR 1
#define ECL_VERSION_MINOR 0
#define ECL_VERSION_PATCH 0
#define ECL_VERSION_NUMBER (ECL_VERSION_MAJOR*10000 + ECL_VERSION_MINOR*100 + ECL_VERSION_PATCH)

#define ECL_VERSION_BRANCH "master"


/* user setup part --------------------------------------------- */
/*
    Add your own definitions here if you don't have available:
    - uint8_t
    - uint16_t
    - uint32_t
*/

#ifndef ECL_USE_BITNESS_16
#ifndef ECL_USE_BITNESS_32
#ifndef ECL_USE_BITNESS_64
/* none of these is defined with force for compilation, use 32 bits by default */
#define ECL_USE_BITNESS_32
#endif
#endif
#endif

/*#define ECL_USE_ASSERT */
/*#define ECL_USE_BRANCHLESS */
/*#define ECL_USE_STAT_COUNTERS */
/*#define ECL_DISABLE_MALLOC */
/*#define ECL_USE_DLL */


#ifndef ECL_NANO_LZ_ONLY_SCHEME
/* set to 0 to unlock all schemes, set to 1 to have only scheme1, 2 = scheme2 etc. Having single scheme allows compiler to inline for better performance */
/* default is 1 since most likely you will use only scheme1 and want better performance */
#define ECL_NANO_LZ_ONLY_SCHEME 1
#endif

/* non-user part ------------------------------------------ */
#define ECL_NANO_LZ_IS_SCHEME_ENABLED(scheme_num) ((ECL_NANO_LZ_ONLY_SCHEME == 0) || (ECL_NANO_LZ_ONLY_SCHEME == scheme_num))

/* size types --------------------------------------------- */
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
    Helper paranoid macro to ensure that estimated compression bound doesn't overflow ECL_usize.
    Results in bool. Makes sense for ECL_USE_BITNESS_16.
    Usage: assert( ECL_VALIDATE_BOUND_OK( ECL_NANO_LZ_GET_BOUND(src_size), src_size ) );
*/
#define ECL_VALIDATE_BOUND_OK(estimated_max_compressed_size, src_size) (( (ECL_usize)(estimated_max_compressed_size) ) > ( (ECL_usize)(src_size) ))


/* asserts --------------------------------------------- */
#ifdef ECL_USE_ASSERT
    #include <assert.h>
    #define ECL_ASSERT(expr) assert(expr)
#else
    #define ECL_ASSERT(expr)
#endif

/* malloc/free can be disabled (stubbed). useful if those functions don't compile on some particular platform */
#ifdef ECL_DISABLE_MALLOC
    #define ECL_MEM_ALLOC(size) 0
    #define ECL_MEM_FREE(ptr)
#else
    #define ECL_MEM_ALLOC(size) malloc(size)
    #define ECL_MEM_FREE(ptr) free(ptr)
#endif


/* helpful */
#define ECL_MIN(a, b) ((a) < (b) ? (a) : (b))
#define ECL_MAX(a, b) ((a) > (b) ? (a) : (b))


/* dyn library macros, etc */
#ifndef ECL_API_VISIBILITY
    #if defined(__GNUC__) && (__GNUC__ >= 4)
        #define ECL_API_VISIBILITY __attribute__ ((visibility ("default")))
    #else
        #define ECL_API_VISIBILITY
    #endif
#endif

#if defined(ECL_USE_DLL)
    #if defined(ECL_DLL_EXPORT)
        #define ECL_EXPORTED_API __declspec(dllexport) ECL_API_VISIBILITY
    #else
        #define ECL_EXPORTED_API __declspec(dllimport) ECL_API_VISIBILITY
    #endif
#else
    #define ECL_EXPORTED_API ECL_API_VISIBILITY
#endif

#endif
